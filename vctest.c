/* vctest.c - test virtual consoles */

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char buf[8192];
char name[3];
char *echoargv[] = { "echo", "ALL", "TESTS", "PASSED", 0 };
int stdout = 1;

int
main(int argc, char *argv[])
{
	// int fds[2], pid;
	// int seq, i, n, cc, total;

	// if (pipe(fds) != 0) {
	// 	printf(1, "pipe() failed\n");
	// 	exit();
	// }
	// pid = fork();
	// seq = 0;
	// if (pid == 0) {
	// 	close(fds[0]);
	// 	for (n = 0; n < 5; n++) {
	// 		for (i = 0; i < 1033; i++) buf[i] = seq++;
	// 		if (write(fds[1], buf, 1033) != 1033) {
	// 			printf(1, "pipe1 oops 1\n");
	// 			exit();
	// 		}
	// 	}
	// 	exit();
	// } else if (pid > 0) {
	// 	close(fds[1]);
	// 	total = 0;
	// 	cc    = 1;
	// 	while ((n = read(fds[0], buf, cc)) > 0) {
	// 		for (i = 0; i < n; i++) {
	// 			if ((buf[i] & 0xff) != (seq++ & 0xff)) {
	// 				printf(1, "pipe1 oops 2\n");
	// 				exit();
	// 			}
	// 		}
	// 		total += n;
	// 		cc = cc * 2;
	// 		if (cc > sizeof(buf)) cc = sizeof(buf);
	// 	}
	// 	if (total != 5 * 1033) {
	// 		printf(1, "pipe1 oops 3 total %d\n", total);
	// 		exit();
	// 	}
	// 	close(fds[0]);
	// 	wait();
	// } else {
	// 	printf(1, "fork() failed\n");
	// 	exit();
	// }
	// printf(1, "pipe1 ok\n");
	// exit();

	// ---------------

	int fd, id;

	if (argc < 3) {
	  printf(1, "usage: vctest <vc> <cmd> [<arg> ...]\n");
	  exit();
	}

	fd = open(argv[1], O_RDWR);
	printf(1, "fd = %d\n", fd);

	/* fork a child and exec argv[1] */
	id = fork();

	if (id == 0) {
	  close(0);
	  close(1);
	  close(2);
	  dup(fd);
	  dup(fd);
	  dup(fd);
	  exec(argv[2], &argv[2]);
	  exit();
	}

	printf(1, "%s started on vc0\n", argv[1]);
	wait();
	exit();
}
