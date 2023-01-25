#include "types.h"

#include "fs.h"
#include "fcntl.h"
#include "helpers.h"

#define stdout 1
#define TEST_ID 1

/* error codes */
int INIT_DIR_ERROR = 0;


/* function declarations */
int cm_create_and_enter(char *specfile);
int cm_setroot(char *path, int path_len);
int enter_container();
int cm_maxproc(int nproc);

int fds[2]; // cm <- coor
int fdz[2]; // cm -> coor

char init[128];
char fs[128];
int  nproc;
int  retcode;

/*
 * This creates a new container, and enters into it.
 * The current process that calls this function will go from not being part of a container
 * (thus using global system namespaces) to being in a container, thus having potentially restricted namespaces.
 * The namespaces for locks, cvs, and shared memory created from this point on are local to this container.
 * One container can try and create a lock called “locky_mc_lockface” and it will find its lock.
 * A different container trying to find the same lock will actually create a different lock (as there are per-container
 * namespaces). Once this process is terminated the container is destroyed. Define your own appropriate error conditions
 * and values.
 */

int
cm_create_and_enter(char *specfile)
{
	
	// parse spec.json
	if ((retcode = parse_spec(specfile, init, fs, &nproc)) != 0) { return retcode; }

	// create container index
	int cid = get_new_cid();
	// printf(1, "cid = %d\n", 0);

	if (cid < 0) {
		printf(1, "Maximum number of containers has been reached!\n");
		return cid;
	}
	
	printf(1, "CM creating container with init = %s, root fs = %s, and max num processes = %d\n", init, fs, nproc);

	// init directory
	if (init_dir(fs) <= 0) { return INIT_DIR_ERROR; }



	setname(cid, fs);
	setalive(cid, 1);
	// cm_maxproc(nproc);
	setmaxproc(cid, nproc);


	/* fork a child and exec init */

	if (pipe(fds) != 0) {
		printf(1, "pipe() failed fds\n");
		return -1;
	}

	if (pipe(fdz) != 0) {
		printf(1, "pipe() failed fdz\n");
		return -1;
	}
	// printf(1, "fds: %d, %d\n", fds[0], fds[1]);
	// printf(1, "fdz: %d, %d\n", fdz[0], fdz[1]);

	int pipefd[2];
	pipefd[0] = fdz[0]; // pipe in for coor
	pipefd[1] = fds[1]; // pipe out for coor
	setvc(cid, fs, pipefd);
	if (fork2(cid) == 0) {
		/* Child process closes up input side of pipe */
		close(fds[0]);
		close(fdz[1]);

		// set container root
		setroot(cid, fs);
		if (chdir("/") < 0) {
			printf(1, "Container %s does not exist.", fs);
			return -1;
		}
		char *inits[2] = {init, 0};
		exec(inits[0], inits);
		return 0;
	}

	/* Parent process closes up output side of pipe */
	close(fds[1]);
	close(fdz[0]);
	close(fds[0]);
	close(fdz[1]);
	wait();

	return 0;
}

int
io_container()
{
	// parse spec.json
	if ((retcode = parse_spec("spec.json", init, fs, &nproc)) != 0) { return retcode; }

	// create container index
	int cid = get_new_cid();
	// set active file share
	setname(cid, fs);
	setalive(cid, 1);
	// cm_maxproc(nproc);
	setmaxproc(cid, nproc);


	/* fork a child and exec init */

	if (pipe(fds) != 0) {
		printf(1, "pipe() failed fds\n");
		return -1;
	}

	if (pipe(fdz) != 0) {
		printf(1, "pipe() failed fdz\n");
		return -1;
	}
	// printf(1, "fds: %d, %d\n", fds[0], fds[1]);
	// printf(1, "fdz: %d, %d\n", fdz[0], fdz[1]);

	int pipefd[2];
	pipefd[0] = fdz[0]; // pipe in for coor
	pipefd[1] = fds[1]; // pipe out for coor
	setvc(cid, fs, pipefd);

	if (fork2(cid) == 0) {
		/* Child process closes up input side of pipe */
		close(fds[0]);
		close(fdz[1]);

		// set container root
		setroot(cid, fs);
		if (chdir("/") < 0) {
			printf(1, "Container %s does not exist.", fs);
			return -1;
		}
		char *inits[2] = {init, 0};
		exec(inits[0], inits);
		return 0;
	}

	/* Parent process closes up output side of pipe */
	close(fds[1]);
	close(fdz[0]);
	close(fds[0]);
	close(fdz[1]);
	wait();
	return 0;
}

int io_pipe(char * shm) {

	int cid = getcid();
	// printf(1, "io cid = %d\n", cid);
	char str[1028];
	get_con_shm(cid, str);
	printf(1, "%s\n", str);

	char * cmd = shm;
	write(fdz[1], cmd, (strlen(cmd) + 1));

	char strout[512];
	read(fds[0], strout, sizeof(strout));
	read(fds[0], strout, sizeof(strout));
	printf(1, "%s\n", strout);
	memset(strout, 0, sizeof(strout));

	close(fds[0]);
	close(fdz[1]);

	return 0;
}

/*
 * Change the root directory for the current process to the given path (the path string has length path_len).
 * The path has to be absolute (i.e. its first character must be /).
 * Any future file system system calls (e.g. open) will progress treating that path as the root.
 * You must prevent the use of “..“ to go “under” the root.
 * This can only be called when the current process is executing in a new container
 * (i.e. when cm_create_and_enter has been called by this process, or one of its parents).
 * Should switch the current process’ current working directory to this new root.
 * Returns an error appropriately.
 */
int
cm_setroot(char *path, int path_len)
{
	// making the path to the new root
	int check = mkdir(path);
	// if mkdir failed, return -1
	if (check < 0) {
		printf(1, "ERROR: Could not make directory \n");
		return check;
	}
	return 0;
}

/*
 * This sets the maximum number of processes (including this process) that can exist in a process tree of descendents
 * rooted at the current process. If a process has already used this function, it or its descendents, cannot use it
 * again (if they do, it returns an error). Additionally, for simplicity, you can assume that the process that calls
 * this system call will not terminate until all of their children have exited. This can only be called when the current
 * process is executing in a new container. Returns an error code appropriately.
 */

int
cm_maxproc(int nproc)
{
	// TODO: fix this
	setmaxproc(0, nproc);
	return 0;
}
