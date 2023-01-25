#include "types.h"
#include "user.h"


int child() {
	mutex_lock(0);
	printf(1, "child cs\n");

	int * a = 0;
	printf(1, "%d", *a);
	
	sleep(200);
	printf(1, "child unlock...\n");
	mutex_unlock(0);

	exit();
	return 0;
}


int main(void) {

	mutex_create("mux"); // 0 

	if (fork() == 0) {
		child();
	}

	sleep(100);

	mutex_lock(0);
	printf(1, "parent cs\n");
	sleep(100);
	printf(1, "parent unlock..\n");

	mutex_unlock(0);
	
	for (int i = 0; i < 3; i++) wait();

	sleep(100);
	exit();
	return 0;
}