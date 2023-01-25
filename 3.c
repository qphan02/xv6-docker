#include "types.h"
#include "user.h"

// TEST CONDITION VARIABLES

int child1(void) {
	printf(1, "child1 waiting for signal\n");
	cv_wait(0);
	printf(1, "child1 LETS GOOO\n");

	return 0;
}

int child2(void) {
	printf(1, "child2 waiting for signal\n");
	cv_wait(1);
	printf(1, "child2 LETS GOOO\n");

	return 0;
}

int main(void) {
	mutex_create("ahmet"); // 0
	mutex_create("mehmet"); // 1

	int pid = fork();
	if (pid == 0) { // child
		child1();
		exit();
	}

	int pid2 = fork();
	if (pid2 == 0) { // child
		child2();
		exit();
	}

	sleep(100);
	printf(1, "signaling on mutex 0...\n");
	cv_signal(0);

	sleep(200);
	printf(1, "signaling on mutex 1...\n");
	cv_signal(1);

	for (int i =0; i < 10; i++) {
		wait();
	}

	exit();
	return 0;
}