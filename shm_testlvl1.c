#include "types.h"
#include "user.h"


int main()
{
	char *shm = shm_get("first");
	char *word = "hello world";

	strcpy(shm,word);
	printf(1, "value before: %s\n" , shm);

	if(fork() == 0){
		char *shm2 = shm_get("second");
		printf(1, "value child before: %s\n" , shm2);
		char *word2 = "bye world";
		strcpy(shm2,word2);
		printf(1, "value child after: %s\n" , shm2);
		shm_rem("second");
		exit();
	}
	sleep(10);
	printf(1, "value after: %s\n" , shm);
	wait();
	shm_rem("first");
	exit();
}