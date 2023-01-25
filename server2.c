#include "types.h"
#include "user.h"


int sender(char* shm) {
	int n = 4;
	char messages[4][10] = {"Hi!","How", "Are", "You"};

	int mux_send = atoi(shm);
	memset(shm,0,sizeof(shm));
	for (int i = 0; i < n; i++) {
		mutex_lock(mux_send);
		strcpy(shm, messages[i]);
		cv_signal(mux_send);
		mutex_unlock(mux_send);
		sleep(200);
	}
	exit();
	return 0;
}
int receiver(char* shm) {
	int mux_receive = atoi(shm);
	memset(shm,0,sizeof(shm));
	while (1) {
		mutex_lock(mux_receive); // i want the lock
		cv_wait(mux_receive);   // i will wait for a signal
		printf(1, "message received: %s\n", shm);
		mutex_unlock(mux_receive);
	}
	exit();
	return 0;
}
int main(void) {

	int mux_send = mutex_create("send"); // 0 
	int mux_receive = mutex_create("receive"); // 1

	char *shm_send = shm_get("send");
	char *shm_receive = shm_get("receive");

	// tell the sender and receiver their repsect muxid
	char mux_send_str[3];
	char mux_receive_str[3];
	itoa(mux_send,mux_send_str,10);
	itoa(mux_receive,mux_receive_str,10);
	strcpy(shm_send, mux_send_str);
	strcpy(shm_receive, mux_receive_str);

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