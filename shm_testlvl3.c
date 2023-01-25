#include "types.h"
#include "user.h"


int main()
{

	if(fork() == 0){
		char *shm2 = shm_get("third");
		char *word2 = "bye world";
		strcpy(shm2,word2);
		printf(1, "value child: %s\n" , shm2);
		shm_rem("third");
		exit();
	}
	char *shm = shm_get("third");
	wait();
	printf(1, "value parent: %s\n" , shm);
	shm_rem("third");
	exit();
}