#include "types.h"
#include "user.h"


int sender(char* shm) {
	int n = 4;
	char messages[4][10] = {"Hi!","How", "Are", "You"};

	for (int i = 0; i < n; i++) {
		mutex_lock(0);
		strcpy(shm, messages[i]);
		cv_signal(0);
		mutex_unlock(0);

		sleep(200);
	}

	exit();
	return 0;
}

int receiver(char* shm) {

	while (1) {
		mutex_lock(1);
		cv_wait(1);
		printf(1, "message received: %s\n", shm);
		mutex_unlock(1);
	}

	exit();
	return 0;
}


int main(void) {
	int mux_send = mutex_create("send"); // 0 
	int mux_receive = mutex_create("receive"); // 1

	char *shm_send = shm_get("send");
	char *shm_receive = shm_get("receive");

	if (fork() == 0) sender(shm_send);
	if (fork() == 0) receiver(shm_receive);
	
	while(1) {
		char message[10];

		mutex_lock(mux_send);
		cv_wait(mux_send);
		strcpy(message, shm_send);
		mutex_unlock(mux_send);
		
		mutex_lock(mux_receive);
		strcpy(shm_receive, message);
		cv_signal(mux_receive);
		mutex_unlock(mux_receive);
	}
	
	for (int i = 0; i < 2; i++) wait();

	sleep(100);
	exit();
	return 0;
}