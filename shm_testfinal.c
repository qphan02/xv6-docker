#include "types.h"
#include "user.h"


int main()
{

	if(fork() == 0){
		sleep(10);
		char *shm2 = shm_get("third");
		char *word2 = "hello world";
		strcpy(shm2,word2);
		
		char *shm3 = shm_get("fourth");
		char *word3 = "bye world";
		char * a = malloc(10);
		strcpy(shm3,word3);
		printf(1, "value child: %s\n" , shm2);
		printf(1, "second value child: %s\n" , shm3);
		shm_rem("third");
		shm_rem("fourth");
		free(a);
		exit();
	}

	char *shm = shm_get("third");
	wait();
	printf(1, "value parent: %s\n" , shm);
	shm_rem("third");
	exit();
}