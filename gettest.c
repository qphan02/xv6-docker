#include "user.h"


int child(void) {

	sleep(100);

	int mux0 = mutex_create("adfsasd");
	int mux1 = mutex_get("lock");

	assert(mux0 == 0);
	assert(mux1 == 1);

	sleep(100);


	mutex_lock(mux1);
	printf(1, "child cs\n");
	sleep(200);
	printf(1, "child unlock\n");
	mutex_unlock(mux1);


	exit();
	return 0;
}

int main(void) {


	if (fork() == 0) {

		child();
	}


	int mux0 = mutex_create("lock");
	assert(mux0 == 0);

	sleep(100);


	mutex_lock(mux0);
	printf(1, "parent cs\n");
	sleep(200);
	printf(1, "parent unlock\n");
	mutex_unlock(mux0);


	wait();
	exit();
	return 0;
}