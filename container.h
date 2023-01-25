#ifndef GUARD
#include "spinlock.h"
#define GUARD
#endif

#define MAX_NUM_CONTAINERS 10

struct container {
	int    alive;
	char   name[32];
	char   vc[32];
	char   path[256];
	int    max_proc;
	int    next_proc;
	char   root_path[128];
	int    active_processes;
	char * shm;
	int    shm_index;

	int				  pipein;
	int				  pipeout;
};

struct {
	struct container containers[MAX_NUM_CONTAINERS];
	struct spinlock  lock;
} manager;

int getname(int index, char *name);

int setname(int index, char *name);

int getmaxproc(int index);

int setmaxproc(int index, int max_proc);

int setvc(int index, char *vc, int * fds);

int tostring(void);

int getpath(int index, char *path);

int setpath(int index, char *path, int remove);

int setalive(int index, int val);

int getnumcontainers(void);

int setroot(int cid, char *root_path);

int get_con_shm(int cid, char * str);

int get_new_cid(void);

int print(char * str, int n,...);

int setnextproc(int index, int val);