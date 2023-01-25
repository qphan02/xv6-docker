#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "container.h"

int
sys_fork(void)
{
  return fork();
}

int 
sys_fork2(void)
{
  int cid;

  if(argint(0, &cid) < 0) {
    return -1;
  }

  return fork2(cid);
}

int 
sys_setalive(void)
{
  int index;
  int val;

  if(argint(0, &index) < 0 || argint(1, &val) < 0) {
    return -1;
  }

  return setalive(index, val);
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n) {
    if(myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_getname(void)
{
  int index;
  char *name;

  if(argint(0, &index) < 0 || argstr(1, &name) < 0) {
    return -1;
  }

  return getname(index, name);
}

int
sys_setname(void)
{
  int index;
  char *name;

  if(argint(0, &index) < 0 || argstr(1, &name) < 0) {
    return -1;
  }

  return setname(index, name);
}

int
sys_setroot(void)
{
  int index;
  char *name;

  if(argint(0, &index) < 0 || argstr(1, &name) < 0) {
    return -1;
  }

  return setroot(index, name);
}

int
sys_getmaxproc(void)
{
  int index;

  if(argint(0, &index) < 0) {
    return -1;
  }

  return getmaxproc(index);
}

int
sys_setmaxproc(void)
{
  int index, max;

  if(argint(0, &index) < 0 || argint(1, &max)) {
    return -1;
  }

  return setmaxproc(index, max);
}


int
sys_setvc(void) {
  int index;
  char *vc;
  int * fds;

  if(argint(0, &index) < 0 || argstr(1, &vc) < 0) {
    return -1;
  }

  if (argptr(2, (void *)&fds, sizeof(*fds) < 0)) return -1;
  return setvc(index, vc, fds);
}

int
sys_tostring(void) {
  
  return tostring();
}

int
sys_getpath(void) {
  int index;
  char *path;

  if(argint(0, &index) < 0 || argstr(1, &path) < 0) {
    return -1;
  }

  return getpath(index, path);
}

int
sys_setpath(void) {
  int index, remove;
  char *path;

  if(argint(0, &index) < 0 || argstr(1, &path) < 0 || argint(2, &remove) < 0) {
    return -1;
  }

  return setpath(index, path, remove);
}

int
sys_ps(void) {
  ps();
  return 0;
}
int
sys_getnumcontainers(void) {
  return getnumcontainers();
}
int sys_prio_set(void)
{
	int pid;
	int priority;
	if (argint(0, &pid) < 0) return -1;
	if (argint(0, &priority) < 0) return -1;
	return prio_set(pid, priority);
}

int
sys_remove_node(void) {
	struct proc *curproc;
	if (argptr(0, (void *)&curproc, sizeof(*curproc) < 0)) return -1;

	return remove_node(curproc);
}

int
sys_shm_get(void)
{
	int addr;
	char *name;

	if (argstr(0, &name) < 0) return 0;

	addr = myproc()->sz;

	char *ret = shm_get(name);

	if(ret == 0){
		return 0;
	}

	return addr;
}

int
sys_shm_rem(void)
{
	char *name;

	if (argstr(0, &name) < 0) return -1;

	int ret = shm_rem(name);

	return ret;
}

int
sys_mutex_create(void) {
	char *name;
	
	if (argptr(0, (void *)&name, sizeof(*name) < 0)) return -1;
	return mutex_create(name);
}

int
sys_mutex_delete(void) {
	int muxid;

	if (argint(0, &muxid) < 0) return -1;
	return mutex_delete(muxid);
}

int
sys_mutex_lock(void) {
	int muxid;

	if (argint(0, &muxid) < 0) return -1;
	return mutex_lock(muxid);
}

int
sys_mutex_unlock(void) {
	int muxid;

	if (argint(0, &muxid) < 0) return -1;
	return mutex_unlock(muxid);
}

int
sys_cv_wait(void) {
	int muxid;

	if (argint(0, &muxid) < 0) return -1;
	return cv_wait(muxid);
}

int
sys_cv_signal(void) {
	int muxid;

	if (argint(0, &muxid) < 0) return -1;
	return cv_signal(muxid);
}

int
sys_mutex_get(void) {
	char *name;

	if (argptr(0, (void *)&name, sizeof(*name) < 0)) return -1;
	return mutex_get(name);
}


int sys_con_shm(void) {
  int cid;
  char * str;
  if (argint(0, &cid) < 0) return -1;
  if (argstr(0, &str) < 0) return -1;

  return get_con_shm(cid, str);
}
