#include "types.h"
#include "user.h"


int customer(int no) {

	printf(1, "[customer %d] arrived!\n", no);
	for (int i = 0; i < 3; i++) {
		mutex_lock(0);
		cv_wait(0);
		printf(1, "[customer %d] got the food [%d/3]\n", no, i+1);
		mutex_unlock(0);
	}

	exit();
	return 0;
}


int main(void) {
	mutex_create("food"); // 0
	printf(1, "[restaurant] opening...\n");

	printf(1, "[restaurant] 1 food ready for the earliest customer!\n");
	mutex_lock(0);
	cv_signal(0);
	mutex_unlock(0);

	sleep(100);
	if (!fork()) customer(1);
	if (!fork()) customer(2);
	if (!fork()) customer(3);
	sleep(20);
	printf(1, "\n");

	for (int i = 0; i < 11; i++) {
		sleep(300); // cooking is no easy job
		mutex_lock(0);
		printf(1, "[restaurant] food ready!\n");
		cv_signal(0);
		mutex_unlock(0);
	}

	printf(1, "\n[restaurant] closing!!! see you tomorrow!\n");
	for (int i = 0; i < 3; i++) wait();
	exit();
	return 0;
}