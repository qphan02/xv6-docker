#include "fcntl.h"
#include "types.h"
#include "stat.h"
#include "cm.h"

int main(void) {
	printf(1, "Container Manager starting...\n");
	// create shms
	char *shm_send 		= shm_get("ch1"); // cm -> dockv6
	char *shm_receive 	= shm_get("ch2"); // cm <- dockv6

	int mux_send 			= mutex_create("send"); 
	int mux_receive 		= mutex_create("receive");
	printf(1, "CM awaiting request...\n");

	// dockv6 create & io
	while(1) {
		char buf[1024];
		if (strlen(shm_send) == 0) exit();
		// waiting request from dockv6
		mutex_lock(mux_receive);
		cv_wait(mux_receive);
		strcpy(buf, shm_receive);
		printf(1,"received: %s\n",buf);
		mutex_unlock(mux_receive);

		char rspn[512] = "rspn\n";
		const char s[2] = " ";
		char * token;
		char * args[2];
		int i = 0;
		token = strtok(buf, s);
		while( token != NULL ) {
			args[i++] = token;
			token = strtok(NULL, s);
		}

		char * mode = args[0];
		char * arg1 = args[1];

		if (strcmp(mode,"create") == 0) {
			cm_create_and_enter(arg1);
		} else if (strcmp(mode,"io") == 0) {
			io_container(0);
		}
		mutex_lock(mux_send);
		strcpy(shm_send, rspn);
		printf(1,"cm send: %s\n", shm_send);
		cv_signal(mux_send);
		mutex_unlock(mux_send);

		sleep(5);
	} 

	exit();
}