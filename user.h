#include "types.h"
struct stat;
struct rtcdate;
struct proc;

// System calls
int fork();
int fork2(int);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int getname(int, char*);
int setname(int, char*);
int getmaxproc(int);
int setmaxproc(int, int);
int setvc(int, char*, int *);
int getcwd(void*, int);
int tostring(void);
int getpath(int, char*);
int setpath(int, char*, int);
int setalive(int, int);
int ps(void);
int getnumcontainers(void);
int setroot(int cid, char * root_path);
int get_con_shm(int cid, char * str);
int print(char * str, int fd,...);

char *shm_get(char *name);
int   shm_rem(char *name);
int   mutex_create(char *name);
int   mutex_get(char *name);
int   mutex_delete(int muxid);
int   mutex_lock(int muxid);
int   mutex_unlock(int muxid);
int   cv_wait(int muxid);
int   cv_signal(int muxid);

int   prio_set(int pid, int priority);
int   remove_node(struct proc *curproc);
void  print_priorities(void);
void  print_status(void);
void  test_0(void);
void  test_1(void);
void  test_2(void);


// ulib.c
int   stat(char *, struct stat *);
char *strcpy(char *, char *);
void *memmove(void *, void *, int);
char *strchr(const char *, char c);
int   strcmp(const char *, const char *);
void  printf(int, char *, ...);
char *gets(char *, int max);
void *memset(void *, int, uint);
void *malloc(uint);
void  free(void *);
int   atoi(const char *);

// ulib.c
int stat(char*, struct stat*);
char *strcpy(char*, char*);
void *memmove(void*, void*, int);
char *strchr(const char*, char c);
int strcspn(const char *, const char *);
int strcmp(const char*, const char*);
int strncmp(const char*, const char *, uint);
int strstr(char*, char*);
char *strcat(char*, const char*);
char *strtok(char*, const char*);
uint strtoul(char*, char**, int);
int strtol(char*, char**, int);
int isspace(unsigned char);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
void itoa(int, char*, int);
char *spaces(int, char[]);
int copy(char*, char*);
int isfscmd(char*);
int addedcpath(char*);
int ifsafepath(char*);
int getcid(void);
int get_new_cid(void);
int get_proc_pipe(int * fds);

#define assert(expr) \
    if (!(expr)) \
        aFailed(__FILE__, __LINE__)

static void aFailed(char *file, int line) __attribute__((unused));
static void aFailed(char *file, int line){
	printf(2, "\nUser Assertion failed in %s on line %d\n", file, line);
	exit();
}
