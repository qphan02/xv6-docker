#include "types.h"

#include "fs.h"
#include "fcntl.h"
#include "user.h"


int fd[2];

void child() {
	// char string[] = "Hello World\n";
	// write(fd[1], string, (strlen(string)+1));

	char    readbuffer[512];
	int n;
	char str[1024];

	while(1) {
		n = read(fd[0], readbuffer, sizeof(readbuffer));
		printf(1,"%d ",n);
		if (n <=  0) break;
		strcat(str, readbuffer);
		printf(1,"readbuffer: %s\n", readbuffer);
		sleep(50);
		char a = readbuffer[strlen(readbuffer)-1];
		if (a=='\n' || a=='\r') {
			break;
		}
	}
	printf(1,"child: %s\n", str);
}

int main() {
	int fds[2];
	int fdz[2];

	pipe(fds);
	pipe(fdz);

	fd[0] = fdz[0];
	fd[1] = fds[1];
	int pid = fork();
	if (pid==0) {
		close(0);
		// close(1);
		close(fds[0]);
		close(fdz[1]);
		child();
		exit();
	}

	close(fds[1]);
	close(fdz[0]);


	char cmd[] = "ls hello world\n\n\n hello world 2\n";
	write(fdz[1], cmd, (strlen(cmd)+1));
	wait();
	exit();

	// char    readbuffer[512];
	// int n;
	// while((n = read(fds[0], readbuffer, sizeof(512))) > 0) {
	// 	printf(1,"Parent: %s\n", readbuffer);
	// }
}