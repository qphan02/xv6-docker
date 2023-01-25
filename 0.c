#include "types.h"
#include "user.h"

// LEVEL 0: ALLOCATE AND FREE MUTEXES

int child1(void) {
	int mux1 = mutex_create("child1 mux1");
	assert(mux1 == 1);

	mutex_lock(mux1);
	printf(1, "child1 mux1 cs\n");
	sleep(200);
	mutex_unlock(mux1);
	printf(1, "child1 unlocked mux1\n");

	exit();
	return 0;
}

int child2(void) {
	mutex_lock(1);
	printf(1, "child2 mux1 cs\n");
	sleep(200);
	mutex_unlock(1);
	printf(1, "child2 unlocked mux1\n");

	exit();
	return 0;
}


int main(void) {
	int mux0 = mutex_create("mux0");
	int mux1 = mutex_create("mux1");
	int mux2 = mutex_create("mux2");
	assert(mux0 == 0);
	assert(mux1 == 1);
	assert(mux2 == 2);

	mutex_delete(mux1);
	assert(mutex_delete(1) == -1);
	assert(mutex_lock(1) == -1);
	assert(mutex_unlock(1) == -1);

    ///////////////////// 
	// if (!fork()) {
	// 	child1();
	// }

	// mux1 = mutex_create("mux1 new\n");
	// assert(mux1 == 1);

	// mutex_lock(mux1);
	// printf(1, "parent mux1 cs\n");
	// sleep(200);
	// printf(1, "parent unlocked mux1\n");
	// mutex_unlock(mux1);
	/////////////////////


	///////////////////// 
	// mux1 = mutex_create("mux1 new\n");
	// assert(mux1 == 1);
	// if (!fork()) child2();

	// mutex_lock(mux1);
	// printf(1, "parent mux1 cs\n");
	// sleep(200);
	// printf(1, "parent unlocked mux1\n");
	// mutex_unlock(mux1);
	/////////////////////


	wait();
	wait();
	// printf(1, "PASSED!\n");
	exit();
	return 0;
}