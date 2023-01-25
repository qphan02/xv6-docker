#include "types.h"
#include "user.h"

// TEST MUTUAL EXCLUSION AMONG THREE PROCESSES

int child1(void) {
	mutex_lock(0);
	printf(1, "child1 cs\n");
	sleep(200);
	printf(1, "child1 unlocked\n");
	mutex_unlock(0);

	exit();
	return 0;
}

int child2(void) {
	mutex_lock(0);
	printf(1, "child2 cs\n");
	sleep(200);
	printf(1, "child2 unlocked\n");
	mutex_unlock(0);

	exit();
	return 0;
}


int main(void) {
	int muxid = mutex_create("mux");
	assert(muxid == 0);

	int pid = fork();
	if (pid == 0) { // child
		child1();
	}
	int pid2 = fork();
	if (pid2 == 0) { // child
		child2();
	}

	sleep(100);
	mutex_lock(0);
	printf(1, "parent cs\n");
	mutex_unlock(0);
	printf(1, "parent unlocked\n");


	wait();
	wait();
	exit();
	return 0;
}