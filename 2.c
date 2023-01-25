#include "types.h"
#include "user.h"

// #define 

// TEST COPYING MUTEXES WITH FORK()

int child1(void) {
	int m1 = mutex_create("1");
	printf(1, "m1: %d\n", m1);

	mutex_lock(0);
	printf(1, "child1 cs\n");
	sleep(200);
	// mutex_delete(m1);
	printf(1, "child1 unlocked\n");
	mutex_unlock(0);

	return 0;
}

int child2(void) {
	int m2 = mutex_create("1");
	printf(1, "m2: %d\n", m2);

	mutex_lock(0);
	printf(1, "child2 cs\n");
	sleep(200);
	printf(1, "child2 unlocked\n");
	mutex_unlock(0);

	return 0;
}


int main(void) {
	int muxid = mutex_create("ahmet");
	printf(1, "muxid: %d\n", muxid);

	int pid = fork();
	if (pid == 0) { // child
		child1();
		int pid2 = fork();
		if (pid2 == 0) { // child
			child2();
			exit();
		}
		exit();
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