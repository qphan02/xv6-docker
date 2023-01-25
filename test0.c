#include "types.h"
#include "user.h"
#include "test.h"

void
child(void)
{
	test_0();
}

int main(void){
	if(fork() == 0){
		child();
		exit();
	}
	wait();
	exit();
}