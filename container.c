#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "container.h"

int
strcmp(const char *p, const char *q)
{
	while (*p && *p == *q) p++, q++;
	return (char)*p - (char)*q;
}

int
rootcmp(const char *p, const char *q)
{
	while (*p && *p == *q) p++, q++;
	if (*p == 0) return 0;
	return (char)*p - (char)*q;
}

int
getname(int index, char *name)
{
	int i = 0;
	acquire(&manager.lock);
	while ((*name++ = manager.containers[index].name[i++]) != 0)
		;
	release(&manager.lock);

	return 0;
}

int
setname(int index, char *name)
{
	int i = 0;
	acquire(&manager.lock);
	while ((manager.containers[index].name[i++] = *name++) != 0)
		;
	release(&manager.lock);

	return 0;
}

int
getmaxproc(int index)
{
	acquire(&manager.lock);
	int max_proc = manager.containers[index].max_proc;
	release(&manager.lock);
	return max_proc;
}

int
setmaxproc(int index, int max_proc)
{
	acquire(&manager.lock);
	manager.containers[index].max_proc = max_proc;
	release(&manager.lock);
	return 0;
}

int
setalive(int index, int val)
{
	manager.containers[index].alive = val;
	return 0;
}

int
getalive(int index)
{
	return manager.containers[index].alive;
}

int
getpath(int index, char *path)
{
	int i = 0;
	acquire(&manager.lock);
	while ((*path++ = myproc()->path[i++]) != 0)
		;
	release(&manager.lock);

	return 0;
}

int
getnumcontainers()
{
	int i = 0, count = 0;
	while (i++ != MAX_NUM_CONTAINERS) {
		if (manager.containers[i].name != 0) { count++; }
	}
	return count;
}

// update = 0  -> no update
// update = 1  -> do update
// update = 2  -> special uses for ls and exec
// update = 3  -> process
int
setpath(int index, char *path_addr, int update)
{
	int   i = 1, j, x, single = 1;
    char  path[128];
	char  temp_currpath[128];
	char  temp_path[128];
	char *token_cab, *token_path;
	char *path_arr[128];
	char  orginal_pwd_path[128]; // for ls purpose

	memset(path_arr,0,sizeof(path_arr));

    strcpy(path, path_addr);

	// if cid < 0 then automate find cid given the current process
	if (index < 0) { index = myproc()->cid; }

	// checking to see if path has multiple directory changes: cd ../../../work
	for (x = 0; x < strlen(path); x++) {
		if (path[x] == '/') { single = 0; }
	}
	// check if path[0] = '/' and remove it
	if (path[0] == '/') {
        strcpy(temp_path, manager.containers[index].root_path);
		strcat(temp_path, path + 1);
		strcpy(path, temp_path);
		memset(temp_path,0,sizeof(temp_path));
	}
	// check if path[-1] = '/' and remove it
	if (path[strlen(path) - 1] == '/' && strlen(path) > 1) { path[strlen(path) - 1] = 0; }

    if (strcmp(path,".") == 0) return 0;

	if (update >= 1) {
		strcpy(orginal_pwd_path,  myproc()->path); // for ls
		strcpy(temp_currpath, myproc()->path);
		strcat(temp_currpath, "\0");
		token_cab = temp_currpath;
		strcpy(temp_path, path_addr);
		strcat(temp_path, "\0");
		token_path  = temp_path;
		path_arr[0] = "/";

		path_arr[i] = strtok(token_cab, "/");
		// building array from current path: path_arr = ["/", "c0", ..., ...]
		while (path_arr[i] != 0) { path_arr[++i] = strtok(0, "/"); }

		// updating current path array to new path
		if (single == 0) {
			token_path = strtok(token_path, "/");

			while (token_path != 0) {
				if (strcmp(token_path, "..") == 0) {
                    if (i>1) {
					    path_arr[--i] = 0;
                    }
				} else {
					path_arr[i++] = token_path;
				}
				token_path = strtok(0, "/");
			}
		} else {
			if (strcmp(path, "..") == 0) {
                if (i>1) {
				    path_arr[--i] = 0;
                }
			} else {
				path_arr[i] = path;
			}
		}

		// copy back into container path member
		strcpy(myproc()->path, path_arr[0]);
		for (j = 1; j <= i; j++) {
			if (path_arr[j] != 0 && strlen(path_arr[j]) > 0) {
				strcat(myproc()->path, path_arr[j]);
				strcat(myproc()->path, "/");
			}
		}
		strcat(myproc()->path, "\0");
	} else {
		strcpy(myproc()->path, path);
	}

	if (rootcmp(manager.containers[index].root_path, myproc()->path) != 0) {
		if (update == 2) {
			strcpy(myproc()->path, orginal_pwd_path);
		} else {
			strcpy(myproc()->path, manager.containers[index].root_path);
		}
		return -1;
	}
	strcpy(path_addr,myproc()->path);

	if (update == 2) { strcpy(myproc()->path, orginal_pwd_path); }

	return 0;
}

int
setroot(int cid, char *root_path)
{
	acquire(&manager.lock);
	strcpy(manager.containers[cid].root_path, root_path);
	strcat(manager.containers[cid].root_path, "\0");
	strcpy(manager.containers[cid].path, root_path);
	strcat(manager.containers[cid].path, "\0");
	release(&manager.lock);

	strcpy(myproc()->path, root_path);
	return 0;
}

int
setvc(int index, char *vc, int * fds)
{
	manager.containers[index].shm = shm_get(vc);
	manager.containers[index].pipein  = fds[0];
	manager.containers[index].pipeout = fds[1];
	set_proc_pipe(fds[0],fds[1]);
	return 0;
}

int get_new_cid() {
	int i;
	for (i = 0; i <MAX_NUM_CONTAINERS; i++) {
		if (manager.containers[i].alive == 0) {
			return i;
		}
	}
	return -1;
}

int get_con_shm(int cid, char * str) {
	if (0 < cid && cid >= MAX_NUM_CONTAINERS) {
		cprintf("Invalid cid!\n");
	} else if (manager.containers[cid].alive == 0) {
		cprintf("Container %d has not been initialzied!\n",cid);
	}

	strcpy(str, manager.containers[cid].shm);
	// strcat(str, "\0");
	cprintf("shm2 = %d\n",manager.containers[cid].shm);
	// memset(manager.containers[cid].shm,0,sizeof(manager.containers[cid].shm));
	// manager.containers[cid].shm_index = 0;	
	return 0;
}

int
tostring()
{
	int i;
	acquire(&manager.lock);
	for (i = 0; i < MAX_NUM_CONTAINERS; i++) {
		if (manager.containers[i].alive == 0)
			continue;
		cprintf("Name: ");
		cprintf(manager.containers[i].name);
		cprintf("\n");
		cprintf("shm: ");
		cprintf(manager.containers[i].vc);
		cprintf("\n");
		cprintf("CURRENT PATH: ");
		cprintf(manager.containers[i].path);
		cprintf("\n");
		cprintf("MAX PROC:  ");
		cprintf("%d", manager.containers[i].max_proc);
		cprintf("\n");
		cprintf("USED PROC: ");
		cprintf("%d", manager.containers[i].active_processes);
		cprintf("\n");
		cprintf("Index: ");
		cprintf("%d", i);
		cprintf("\n");
		pscontainer(i);
		cprintf("\n\n");
	}
	release(&manager.lock);
	return 0;
}

int print(char * str, int fd,...) {
	// cprintf("str=[%s] fd=[%d]\n",str,fd);
	// cprintf("%s",str);
	return 0;
}

int
setnextproc(int index, int val)
{
	if (index > -1) {
		// if active_processes == max_proc, return -1
		if (manager.containers[index].alive == 1 && manager.containers[index].active_processes > manager.containers[index].max_proc + 1) {
			// cprintf("Error: Number of active processes exceeds the limit!\n");
			return -1;
		}
		manager.containers[index].next_proc = val;
		manager.containers[index].active_processes += 1;
		return 1;
	}
	return 0;
}