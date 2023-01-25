#include "types.h"
#include "user.h"


int main()
{
	char *shm = shm_get("exec1");

	char *word = "bye world";

	printf(1, "value before in exec: %s\n" , shm);
	strcpy(shm,word);
	printf(1, "value after in exec: %s\n" , shm);
	shm_rem("exec1");
	exit();
}