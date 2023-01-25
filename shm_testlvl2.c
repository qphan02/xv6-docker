#include "types.h"
#include "user.h"


int main()
{
	char *shm = shm_get("second");
	char *word = "hello world";
	
	strcpy(shm,word);
	printf(1, "value before: %s\n" , shm);

	if(fork() == 0){
		printf(1, "value child before: %s\n" , shm);
		char *word2 = "bye world";
		strcpy(shm,word2);
		printf(1, "value child after: %s\n" , shm);
		shm_rem("second");
		exit();
	}
	wait();
	printf(1, "value after: %s\n" , shm);
	shm_rem("second");

	exit();
}