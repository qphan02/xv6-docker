#include "types.h"
#include "user.h"


int main()
{
	char *argv[] = {"shm_testexecfile", 0};
	char *shm = shm_get("exec1");
	char *word = "hello world";
	
	strcpy(shm,word);
	printf(1, "value before: %s\n" , shm);
	if(fork() == 0){
		exec("shm_testexecfile", argv);
		exit();
	}
	
	wait();
	printf(1, "value after: %s\n" , shm);
	shm_rem("exec1");


	exit();
}