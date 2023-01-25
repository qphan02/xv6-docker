#include "types.h"
#include "user.h"
#include "test.h"
#include "param.h"

int
main(void)
{
	test_1();
	int forkret = fork();
	int ret = 0;
	if (forkret == 0) {
		sleep(10);
		exit();
	}
	else {
		ret = prio_set(forkret, 7);
	}

	printf(1, "- Testing for trying to set priority of a process not in ancestry tree\n");
	if (ret == -1) 
		printf(1,"Passed: Process was not in the ancestry tree, priority was not set\n\n");
	else
		printf(1,"Fail\n\n");

	wait();
	exit();
}