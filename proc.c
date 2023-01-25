#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#ifndef GUARD
#include "spinlock.h"
#define GUARD
#endif
#include "container.h"
#define NULL ((void *)0)
#include "shm.h"
#include "mutex.h"
#include "sleeplock.h"

#define PRIO_MAX 10

struct mutex {
	char name[128]; // update when count 0 to 1
	int count;  // 0 default
	int waiting_procs[NPROC]; // all 0 by default,  1 if process is blocked
	int cv_count; // 0 by default, cv_signal() ++  , cv_wait() --
	struct sleeplock slp;
	struct spinlock spin;
};

struct mutex mutexes[MUX_MAXNUM];

struct {
	struct spinlock lock;
	struct proc     proc[NPROC];
} ptable;

struct {
	struct spinlock lock;
	struct proc    *queue[PRIO_MAX];
} prioqueue;

static struct proc *initproc;

int           nextpid   = 1;
unsigned long randstate = 1;

extern void forkret(void);
extern void trapret(void);
int         fork2(int cid);

extern struct shm_info shm_infos[SHM_MAXNUM];

static void wakeup1(void *chan);

void
pinit(void)
{
	initlock(&manager.lock, "manager");
	initlock(&ptable.lock, "ptable");
	for (int i = 0; i < PRIO_MAX; i++) { prioqueue.queue[i] = 0; }
}

// Must be called with interrupts disabled
int
cpuid()
{
	return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void)
{
	int apicid, i;

	if (readeflags() & FL_IF) panic("mycpu called with interrupts enabled\n");

	apicid = lapicid();
	// APIC IDs are not guaranteed to be contiguous. Maybe we should have
	// a reverse map, or reserve a register to store &cpus[i].
	for (i = 0; i < ncpu; ++i) {
		if (cpus[i].apicid == apicid) return &cpus[i];
	}
	panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
	struct cpu  *c;
	struct proc *p;
	pushcli();
	c = mycpu();
	p = c->proc;
	popcli();
	return p;
}

// PAGEBREAK: 32
//  Look in the process table for an UNUSED proc.
//  If found, change state to EMBRYO and initialize
//  state required to run in the kernel.
//  Otherwise return 0.
static struct proc *
allocproc(int cid)
{
	struct proc *p;
	int          i = 0;
	char        *sp;

	acquire(&ptable.lock);

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == UNUSED) {
			goto found;
		}
		i++;
	}

	release(&ptable.lock);
	return 0;

found:
    if (setnextproc(cid, i) < 0) {
		release(&ptable.lock);
		return 0;
	}

	p->state = EMBRYO;
	p->pid   = nextpid++;

	if (p->pid == 1) {
		p->priority = 0;
		pq1_enqueue(p, 0);
	}

	release(&ptable.lock);

	// Allocate kernel stack.
	if ((p->kstack = kalloc()) == 0) {
		p->state = UNUSED;
		return 0;
	}
	sp = p->kstack + KSTACKSIZE;

	// Leave room for trap frame.
	sp -= sizeof *p->tf;
	p->tf = (struct trapframe *)sp;

	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= 4;
	*(uint *)sp = (uint)trapret;

	sp -= sizeof *p->context;
	p->context = (struct context *)sp;
	memset(p->context, 0, sizeof *p->context);
	p->context->eip = (uint)forkret;


	// mutexes
	if (p->pid == 1) {
		for (int i = 0; i < MUX_MAXNUM; i++) {
			p->locks[i] = -1;
		}
	}
	else {
		for (int i = 0; i < MUX_MAXNUM; i++) {
			p->locks[i] = myproc()->locks[i];

			if (p->locks[i] != -1) {
				mutexes[p->locks[i]].count++;
			}
		}
	}
	// p->ticks = 0;

	return p;
}

// PAGEBREAK: 32
//  Set up first user process.
void
userinit(void)
{
	struct proc *p;
	extern char  _binary_initcode_start[], _binary_initcode_size[];

	p = allocproc(-1);

	initproc = p;
	if ((p->pgdir = setupkvm()) == 0) panic("userinit: out of memory?");
	inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
	p->sz = PGSIZE;
	memset(p->tf, 0, sizeof(*p->tf));
	p->tf->cs     = (SEG_UCODE << 3) | DPL_USER;
	p->tf->ds     = (SEG_UDATA << 3) | DPL_USER;
	p->tf->es     = p->tf->ds;
	p->tf->ss     = p->tf->ds;
	p->tf->eflags = FL_IF;
	p->tf->esp    = PGSIZE;
	p->tf->eip    = 0; // beginning of initcode.S
	p->cid        = -1;
	memset(p->path, 0, sizeof(p->path));

	safestrcpy(p->name, "initcode", sizeof(p->name));
	p->cwd     = namei("/");
	p->path[0] = '/';

	// this assignment to p->state lets other cores
	// run this process. the acquire forces the above
	// writes to be visible, and the lock is also needed
	// because the assignment might not be atomic.
	acquire(&ptable.lock);

	p->state = RUNNABLE;

	release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
	uint         sz;
	struct proc *curproc = myproc();

	sz = curproc->sz;
	if (n > 0) {
		if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0) return -1;
	} else if (n < 0) {
		if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0) return -1;
	}
	curproc->sz = sz;
	switchuvm(curproc);
	return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
	return fork2(myproc()->cid);
}

int
fork2(int cid)
{
	int          i, pid;
	struct proc *np;
	struct proc *curproc = myproc();

	// Allocate process.
	if ((np = allocproc(cid)) == 0) { return -1; }

	// Copy process state from proc.
	if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
		kfree(np->kstack);
		np->kstack = 0;
		np->state  = UNUSED;
		return -1;
	}
	strcpy(np->path,curproc->path);  // copy the parent path
	np->sz     = curproc->sz;
	np->cid    = cid;
	np->parent = curproc;
	*np->tf    = *curproc->tf;


	if (np->pid == 1) np->priority = 0;
	else if (np->pid == 2) np->priority = 5;
	else np->priority = np->parent->priority;

	pq1_enqueue(np, np->priority);

	// Clear %eax so that fork returns 0 in the child.
	np->tf->eax = 0;

	for (i = 0; i < NOFILE; i++)
		if (curproc->ofile[i]) np->ofile[i] = filedup(curproc->ofile[i]);
	np->cwd = idup(curproc->cwd);

	safestrcpy(np->name, curproc->name, sizeof(curproc->name));

	pid = np->pid;

	uint addr = np->sz;
	for(int j = SHM_MAXNUM-1; j>=0; j--){
		if(strlen(curproc->shms[j].name) != 0){
			shm_infos[j].ref_count++;
			strncpy(np->shms[j].name, curproc->shms[j].name, strlen(curproc->shms[j].name)+1);
			addr -= PGSIZE;
			np->shms[j].shm_addr = (char *) addr;
		}
	}

	acquire(&ptable.lock);

	np->state = RUNNABLE;

	release(&ptable.lock);

	return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
	struct proc *curproc = myproc();
	struct proc *p;
	int          fd;

	if (curproc == initproc) panic("init exiting");

	// Close all open files.
	for (fd = 0; fd < NOFILE; fd++) {
		if (curproc->ofile[fd]) {
			fileclose(curproc->ofile[fd]);
			curproc->ofile[fd] = 0;
		}
	}

	//Remove shared memory
	for(int j = 0; j<SHM_MAXNUM; j++){
		if(strlen(curproc->shms[j].name) != 0){
			shm_rem(curproc->shms[j].name);
		}
	}

	begin_op();
	iput(curproc->cwd);
	end_op();
	curproc->cwd = 0;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(curproc->parent);

	// Pass abandoned children to init.
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->parent == curproc) {
			p->parent = initproc;
			if (p->state == ZOMBIE) wakeup1(initproc);
		}
	}
	remove_node(curproc);



	for (int i = 0; i < MUX_MAXNUM; i++) {
		int global_id = curproc->locks[i];
		if (curproc->locks[i] != -1) {
			mutexes[global_id].count--;
			if (mutexes[global_id].slp.pid == curproc->pid) {
				pushcli();
				release(&ptable.lock);
				releasesleep(&mutexes[curproc->locks[i]].slp);
				acquire(&ptable.lock);
				popcli();
			}
		}
	}
	

	// Jump into the scheduler, never to return.
	curproc->state = ZOMBIE;
	sched();
	panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
	struct proc *p;
	int          havekids, pid;
	struct proc *curproc = myproc();

	acquire(&ptable.lock);
	for (;;) {
		// Scan through table looking for exited children.
		havekids = 0;
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if (p->parent != curproc) continue;
			havekids = 1;

			if (p->state == ZOMBIE) {
				// Found one.
				pid = p->pid;

				kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);
				p->pid     = 0;
				p->parent  = 0;
				p->name[0] = 0;
				p->killed  = 0;
				p->state   = UNUSED;
				p->priority = -1;

				// decrement number of active process
				if (p->cid >= 0) {
					acquire(&manager.lock); // DEADLOCK CAUTION
					if (manager.containers[p->cid].active_processes > 0)
						manager.containers[p->cid].active_processes -= 1;
					release(&manager.lock);
				}
				release(&ptable.lock);
			
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if (!havekids || curproc->killed) {
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock); // DOC: wait-sleep
	}
}

// PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

// scheduler doesn't know how to return back to the shell from the process

void
scheduler(void)
{
	struct proc *p;
	struct cpu  *c = mycpu();
	c->proc        = 0;
	for (;;) {
		// Enable interrupts on this processor.
		sti();

		// Dequeue from prioqueue for process to run

		acquire(&ptable.lock);
		
		p = pq1_dequeue();

		if (p == NULL)  {
			release(&ptable.lock);
			continue;
		}

		// Switch to chosen process.  It is the process's job
		// to release ptable.lock and then reacquire it
		// before jumping back to us.
		c->proc = p;
		switchuvm(p);
		p->state = RUNNING;

		swtch(&(c->scheduler), p->context);
		switchkvm();

		// Process is done running for now.
		// It should have changed its p->state before coming back.
		c->proc = 0;
	
		release(&ptable.lock);
	}
}

	// Enter scheduler.  Must hold only ptable.lock
	// and have changed proc->state. Saves and restores
	// intena because intena is a property of this
	// kernel thread, not this CPU. It should
	// be proc->intena and proc->ncli, but that would
	// break in the few places where a lock is held but
	// there's no process.
void sched(void)
{
	int          intena;
	struct proc *p = myproc();

	if (!holding(&ptable.lock)) panic("sched ptable.lock");
	if (mycpu()->ncli != 1) panic("sched locks");
	if (p->state == RUNNING) panic("sched running");
	if (readeflags() & FL_IF) panic("sched interruptible");
	intena = mycpu()->intena;
	swtch(&p->context, mycpu()->scheduler);

	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{

	acquire(&ptable.lock); // DOC: yieldlock
	myproc()->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void)
{
	static int first = 1;
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	if (first) {
		// Some initialization functions must be run in the context
		// of a regular process (e.g., they call sleep), and thus cannot
		// be run from main().
		first = 0;
		iinit(ROOTDEV);
		initlog(ROOTDEV);
	}

	// Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
	struct proc *p = myproc();

	if (p == 0) panic("sleep");

	if (lk == 0) panic("sleep without lk");

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.
	if (lk != &ptable.lock) {      // DOC: sleeplock0

		acquire(&ptable.lock); // DOC: sleeplock1
		release(lk);
	}
	// Go to sleep.
	p->chan  = chan;
	p->state = SLEEPING;

	sched();

	// Tidy up.
	p->chan = 0;

	// Reacquire original lock.
	if (lk != &ptable.lock) { // DOC: sleeplock2
		release(&ptable.lock);

		acquire(lk);
	}
}

// PAGEBREAK!
//  Wake up all processes sleeping on chan.
//  The ptable lock must be held.
static void
wakeup1(void *chan)
{
	struct proc *p;

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if (p->state == SLEEPING && p->chan == chan) p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan)
{

	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid)
{
	struct proc *p;

	acquire(&ptable.lock);
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->pid == pid) {
			p->killed = 1;
			// Wake process from sleep if necessary.
			if (p->state == SLEEPING) p->state = RUNNABLE;
			release(&ptable.lock);
			return 0;
		}
	}
	release(&ptable.lock);
	return -1;
}

// PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
	static char *states[] = {[UNUSED] "unused",   [EMBRYO] "embryo",  [SLEEPING] "sleep ",
								[RUNNABLE] "runble", [RUNNING] "run   ", [ZOMBIE] "zombie"};
	int          i;
	struct proc *p;
	char        *state;
	uint         pc[10];

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == UNUSED) continue;
		if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		cprintf("%d %s %s %d", p->pid, state, p->name, p->ticks);
		if (p->state == SLEEPING) {
			getcallerpcs((uint *)p->context->ebp + 2, pc);
			for (i = 0; i < 10 && pc[i] != 0; i++) cprintf(" %p", pc[i]);
		}
		cprintf("\n");
	}
}

int prio_set(int pid, int priority)
{

	acquire(&prioqueue.lock);

	struct proc *curproc = myproc();

	// Must be a valid priority, otherwise return error
	if (priority < 0 || priority >= PRIO_MAX){
		release(&prioqueue.lock);
		return -1;
	} 

	// Cannot raise the priority (numerically lower) higher than current priority
	if (priority < curproc->priority){
		release(&prioqueue.lock);
		return -1;
	}

	// Find the process in the table
	struct proc *setproc = NULL;
	for (int i = 0; i < NPROC; i++) {
		if (ptable.proc[i].pid == pid) { setproc = &ptable.proc[i]; }
	}

	// Must be in the ancestry
	struct proc *ancestor = setproc;
	int          found    = 0;

	while (ancestor != 0) {
		if (ancestor->pid == curproc->pid) {
			found = 1;
			break;
		}
		ancestor = ancestor->parent;
	}

	// If process is not in the ancestry, return error
	if (!found){
		release(&prioqueue.lock);
		return -1;
	}

	// All conditions cleared, remove it from the queue first so no duplicates
	release(&prioqueue.lock);
	remove_node(setproc);

	// set the priority and add it to the queue
	setproc->priority = priority;
	pq1_enqueue(setproc, priority);

	// Success
	return 0;
}

int remove_node(struct proc * curproc) {


	acquire(&prioqueue.lock);

	struct proc *prev = NULL;
	struct proc *cur = prioqueue.queue[curproc->priority];

	while (cur) {
		if (cur->pid == curproc->pid) {
			// Remove node from list
			if (prev) {
				prev->next = cur->next;
			} else {
				prioqueue.queue[curproc->priority] = cur->next;
			}
			cur->next = NULL;
			break;
		}
		prev = cur;
		cur = cur->next;
	}

	release(&prioqueue.lock);
	return 0;
}

void print_priorities(void)
{

	acquire(&prioqueue.lock);

	cprintf("PRIOQUEUE:\n");
	for (int i = 0; i < PRIO_MAX; i++) {
		struct proc *cur = prioqueue.queue[i];
		cprintf("[%d]: ", i);
		if (!cur) {
			cprintf("\n");
			continue;
		};
		while (cur) {
			cprintf("%d ", cur->pid);
			cur = cur->next;
		}
		cprintf("\n");
	}
	cprintf("\n");

	release(&prioqueue.lock);
}

void print_status(void)
{
	struct proc *p;
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->pid != 0) {
			int state = p->state;
			if (state == 0)
				cprintf("PID: %d, Status: UNUSED\n", p->pid);
			else if (state == 1)
				cprintf("PID: %d, Status: EMBRYO\n", p->pid);
			else if (state == 2)
				cprintf("PID: %d, Status: SLEEPING\n", p->pid);
			else if (state == 3)
				cprintf("PID: %d, Status: RUNNABLE\n", p->pid);
			else if (state == 4)
				cprintf("PID: %d, Status: RUNNING\n", p->pid);
			else if (state == 5)
				cprintf("PID: %d, Status: ZOMBIE\n", p->pid);
			else
				cprintf("ERROR :: INVALID STATE\n");
		}
	}
}

void test_0(void)
{
	struct proc *p   = myproc();
	int pid = p->pid;
	
	cprintf("\nBefore changing the priority: \n");
	print_priorities();


	int priority = 6;
	cprintf("Setting PID %d to Priority %d\n", pid, priority);
	prio_set(pid, priority);
	cprintf("\nAfter changing the priority: \n");
	print_priorities();

	cprintf("\n\n-----------------------------------\n\n");

	priority = 7;
	cprintf("Setting PID %d to Priority %d\n", pid, priority);
	prio_set(pid, priority);
	cprintf("\nAfter changing the priority: \n");
	print_priorities();

	cprintf("\n\n-----------------------------------\n\n");

	priority = 8;
	cprintf("Setting PID %d to Priority %d\n", pid, priority);
	prio_set(pid, priority);
	cprintf("\nAfter changing the priority: \n");
	print_priorities();

	cprintf("\n\n-----------------------------------\n\n");

	priority = 9;
	cprintf("Setting PID %d to Priority %d\n", pid, priority);
	prio_set(pid, priority);
	cprintf("\nAfter changing the priority: \n");
	print_priorities();
}

void test_1(void)
{
	struct proc *p        = myproc();
	int          pid      = p->pid;
	int          priority = PRIO_MAX + 1;
	cprintf("\nOriginal: \n");
	print_priorities();
	cprintf("\n- Testing for too large a priority\n");
	int ret2 = prio_set(pid, priority);
	if (ret2 == -1) {cprintf("Passed: Could not change process's priority to %d\n\n", priority);} 
	else 			{cprintf("Fail: Process was added with a priority of %d\n\n", priority);}


	cprintf("- Testing for too small a priority\n");
	priority = -1;
	ret2 = prio_set(pid, priority);
	if (ret2 == -1) {cprintf("Passed:Could not change process's priority to %d\n\n", priority);} 
	else 			{cprintf("Fail\n\n");}

	cprintf("- Testing for priority greater than (numerically smaller than) current process\n");
	priority = 1;
	ret2 = prio_set(pid, priority);
	if (ret2 == -1) {cprintf("Passed: Could not change process %d's priority to %d\n\n", pid, priority);}
	else 			{cprintf("Fail\n\n");}

	cprintf("Final PrioQueue:\n");
	print_priorities();
	cprintf("\n");

}

void test_2(void) { 
	// Enqueueing and Dequeuing and remove_node
	struct proc *p   = myproc();
	prio_set(p->pid, 0);

	int pid2 = fork();
	cprintf("p1 PID: %d, p2 PID: %d\n", p->pid, pid2);

	cprintf("Before Dequeue:\n");
	print_priorities();

	cprintf("After Dequeue:\n");
	print_priorities();

	pq1_dequeue();
	wait();
}

struct proc *pq1_dequeue(void) {
	acquire(&prioqueue.lock);

	// Find the first runnable process in the lowest numerical bucket
	struct proc *cur = NULL;
	for (int i = 0; i < PRIO_MAX; i++) {
		cur = prioqueue.queue[i];
		while (cur && cur->state != RUNNABLE) {
			cur = cur->next;
		}
		if (cur) {
			break;
		}
	}

	// If no runnable process was found, return NULL
	if (!cur) {
		release(&prioqueue.lock);
		return NULL;
	}

	// Remove the runnable process from the queue
	struct proc **head = &prioqueue.queue[cur->priority];
	if (*head == cur) {
		// Special case: process is the head of the queue
		*head = cur->next;
	} else {
		// Find the process in the queue and remove it
		while (*head && (*head)->next != cur) {
		head = &(*head)->next;
		}
		if (*head) {
			(*head)->next = cur->next;
		}
	}

	// Append the runnable process to the end of the queue
	head = &prioqueue.queue[cur->priority];
	while (*head) {
		head = &(*head)->next;
	}
	*head = cur;
	cur->next = NULL;

	release(&prioqueue.lock);
	return cur;
}

void pq1_enqueue(struct proc * p, int priority)
{
	acquire(&prioqueue.lock);

	// if current priority is empty
	if (prioqueue.queue[priority] == 0) {
		prioqueue.queue[priority] = p;
	}
	
	// if there are other processes with same priority
	else {
		struct proc *cur = prioqueue.queue[priority];
		prioqueue.queue[priority] = p;
		p->next = cur;
	}

	release(&prioqueue.lock);

	return;
}
int
mutex_create(char *name) {
	struct proc * curproc = myproc();

	int global_id = -1;
	for (int i = 0; i < MUX_MAXNUM; i++) {
		if (mutexes[i].count == 0) {
			global_id = i;
			break;
		}
	}
	if (global_id == -1) return -1;


	initlock(&mutexes[global_id].spin, "mutex");
	acquire(&mutexes[global_id].spin);
	mutexes[global_id].count++;
	// mutexes[global_id].name = name;
	strncpy(mutexes[global_id].name, name, strlen(name));
	release(&mutexes[global_id].spin);

	// update proc.locks
	int muxid = -1;
	for (int i = 0; i < MUX_MAXNUM; i++) {
		if (curproc->locks[i] == -1) {
			curproc->locks[i] = global_id; // point to global mutex
			muxid = i;
			break;
		}
	}

	initsleeplock(&mutexes[global_id].slp, name);

	return muxid;
}

int 
mutex_get(char *name) {
	struct proc * curproc = myproc();

	// cprintf("name: %s\n",name);		
	int global_id = -1;
	for (int i = 0; i < MUX_MAXNUM; i++) {
		// cprintf("curlen: %d\n", strlen(mutexes[i].name));

		// cprintf("mux name: %s\n",mutexes[i].name);		
		if (strncmp(name, mutexes[i].name, strlen(name)) == 0) {
			global_id = i;
			break;
		}
	}

	if (global_id == -1) return -1;

	int muxid = -1;
	for (int i = 0; i < MUX_MAXNUM; i++) {
		if (curproc->locks[i] == global_id) return -2;

		if (curproc->locks[i] == -1) {
			curproc->locks[i] = global_id; // point to global mutex
			muxid = i;
			break;
		}
	}

	return muxid;
}

int
mutex_delete(int muxid) {
	if (muxid < 0 || muxid >= MUX_MAXNUM) return -1;

	struct proc* curproc = myproc();
	int global_id = curproc->locks[muxid];

	// check
	if (curproc->locks[muxid] == -1) return -1; // no access
	if (mutexes[global_id].slp.pid == curproc->pid) { // caller owns mutex
		mutex_unlock(muxid);
	}

	acquire(&mutexes[global_id].spin);
	mutexes[global_id].count--;
	curproc->locks[muxid] = -1;
	release(&mutexes[global_id].spin);

	return 0;
}

int
mutex_lock(int muxid) {
	if (muxid < 0 || muxid >= MUX_MAXNUM) return -1;

	struct proc* curproc = myproc();
	int global_id = curproc->locks[muxid];

	if (curproc->locks[muxid] == -1) return -1; // no access
	if (mutexes[global_id].slp.pid == curproc->pid) { // proc already owns it ?
		return -1;
	}

	acquiresleep(&mutexes[global_id].slp);

	return 0;
}

void
psroot()
{
	static char *states[] = {[UNUSED] "unused",  [EMBRYO] "embryo", [SLEEPING] "sleep ", [RUNNABLE] "runble",
	                         [RUNNING] "run   ", [ZOMBIE] "zombie"};
	int          i;
	struct proc *p;
	char        *state;
	uint         pc[10];

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == UNUSED) continue;
		if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";

		if (p->cid == -1) {
			cprintf("ROOT: ");
		} else {
			cprintf("Container %d: ", p->cid);
		}
		cprintf("%d %s %s %d", p->pid, state, p->name, p->ticks);
		if (p->state == SLEEPING) {
			getcallerpcs((uint *)p->context->ebp + 2, pc);
			for (i = 0; i < 10 && pc[i] != 0; i++) { cprintf(" %p", pc[i]); }
		}
		cprintf("\n");
	}
}

void
pscontainer(int index)
{
	static char *states[] = {[UNUSED] "unused",  [EMBRYO] "embryo", [SLEEPING] "sleep ", [RUNNABLE] "runble",
	                         [RUNNING] "run   ", [ZOMBIE] "zombie" };
	int          i;
	struct proc *p;
	char        *state;
	uint         pc[10];

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->cid == index) {
			if (p->state == UNUSED) continue;
			if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
				state = states[p->state];
			else
				state = "???";

			cprintf("Container %d: ", p->cid);
			cprintf("%d %s %s %d", p->pid, state, p->name, p->ticks);
			if (p->state == SLEEPING) {
				getcallerpcs((uint *)p->context->ebp + 2, pc);
				for (i = 0; i < 10 && pc[i] != 0; i++) { cprintf(" %p", pc[i]); }
			}
			cprintf("\n");
		}
	}
}

int
ps(void)
{
	int index = 0;
	cprintf("ps index = %d\n",0);
	if (index == -1) {
		psroot();
	} else {
		pscontainer(index);
	}
	return 0;
}

int
mutex_unlock(int muxid) {
	if (muxid < 0 || muxid >= MUX_MAXNUM) return -1;

	struct proc* curproc = myproc();
	int global_id = curproc->locks[muxid];

	if (curproc->locks[muxid] == -1) return -1; // no access
	if (mutexes[global_id].slp.pid != curproc->pid) return -1; // proc doesn't own it

	releasesleep(&mutexes[global_id].slp);

	return 0;
}

int
cv_wait(int muxid) {
	if (muxid < 0 || muxid >= MUX_MAXNUM) return -1;

	struct proc* curproc = myproc();
	int global_id = curproc->locks[muxid];

	if (curproc->locks[muxid] == -1) return -1; // no access
	if (mutexes[global_id].slp.pid != curproc->pid) return -1; // not holding the lock

	acquire(&mutexes[global_id].spin);
	mutexes[global_id].waiting_procs[curproc->pid] = 1;

	while (mutexes[global_id].cv_count == 0) {
		mutex_unlock(muxid);
		sleep(&mutexes[global_id], &mutexes[global_id].spin);
	}
	mutexes[global_id].cv_count--; 

	release(&mutexes[global_id].spin);
	
	return 0;
}

int
cv_signal(int muxid) {
	if (muxid < 0 || muxid >= MUX_MAXNUM) return -1;

	struct proc* curproc = myproc();
	int global_id = curproc->locks[muxid];

	if (curproc->locks[muxid] == -1) return -1; // no access
	if (mutexes[global_id].slp.pid != curproc->pid) return -1; // not holding the lock

	acquire(&mutexes[global_id].spin);

	mutexes[global_id].cv_count++; // 

	for (int i = 0; i < NPROC; i++) {
		mutexes[global_id].waiting_procs[i] = 0;
	}
	wakeup(&mutexes[global_id]); // all of them

	release(&mutexes[global_id].spin);
	return 0;
}

int getcid(void) {
	return myproc()->cid;
}

// root/parent
int set_proc_pipe(int pipein, int pipeout) {
	// cprintf("setpipe: %d, %d\n",pipein, pipeout);
	acquire(&ptable.lock);
	struct proc *p;
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->pid == myproc()->pid) {
			p->pipein = pipein;
			p->pipeout = pipeout;
			break;
		}
	}
	release(&ptable.lock);
	return 0;
}

// child/exec
int get_proc_pipe(int * fds) {
	acquire(&ptable.lock);
	struct proc *p;
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->pid == myproc()->parent->pid) {
			fds[0] = p->pipein;
			fds[1] = p->pipeout;
			myproc()->pipein = fds[0];
			myproc()->pipeout = fds[1];
			break;
		}
	}
	// cprintf("fds: [%d,%d]\n",fds[0],fds[1]);
	release(&ptable.lock);
	// cprintf("getpipe: %d, %d\n",fds[0], fds[1]);
	return 0;
}