#include "types.h"
#include "user.h"


int child(char* shm, char * name) {

	mutex_lock(0);
	sleep(100);
	// printf(1, "\nshm: %s\n", shm);
	// printf(1, "%s printing\n", name);
	strcpy(shm, name);
	cv_signal(0);
	mutex_unlock(0);
	sleep(100);

	exit();
	return 0;
}


int main(void) {
	int muxid = mutex_create("race"); // 0 
	assert(muxid == 0);
	char *shm = shm_get("race");

	if (fork() == 0) child(shm, "ahmet"); 
	if (fork() == 0) child(shm, "mehmet"); 
	if (fork() == 0) child(shm, "ayse"); 
	if (fork() == 0) child(shm, "daniel"); 
	if (fork() == 0) child(shm, "jiyan"); 
	if (fork() == 0) child(shm, "leo"); 
	if (fork() == 0) child(shm, "selman"); 
	if (fork() == 0) child(shm, "hamza"); 
	// for (int i = 0 ; i < 10; i++) {
	// 	if (fork() == 0) {
	// 		child(shm, (i+1)*10);
	// 	}
	// }
	
	char winners[3][10]; 
	for (int i = 0; i < 3; i++) {
		mutex_lock(muxid);
		cv_wait(muxid);
		strcpy(winners[i], shm);
		mutex_unlock(muxid);
	}
	
	// printf(1, "winners are: \n");
	// for (int i = 0; i < 3; i++) {
	// 	printf(1, "%d. %s\n", i+1, winners[i]);
	// }

	for (int i = 0; i < 10; i++) wait();

	sleep(100);
	exit();
	return 0;
}