// test max proc enforement
// can generate as many process as the input

#include "types.h"
#include "user.h"

int main(int argc, char * argv[]) {
	if (argc < 2) {
		printf(1,"Usage: makeproc <number>\n");
		exit();
	}

	int nproc = atoi(argv[1]);
	if (nproc < 1) {
		exit();
	}
	int i;
	for (i=1; i<=nproc; i++) {
		int id = fork();
		if (id < 0) {
			printf(1,"Fork failed!\n",id);
			continue;
		}
		if (id == 0) {
			sleep(5);
			exit();
		}
		printf(1,"Created process pid=%d\n",id);

	}
	while(wait() >= 0);
	printf(1,"All processes exited gracefully!\n");
	exit();
}