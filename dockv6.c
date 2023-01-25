#include "fcntl.h"
#include "types.h"
#include "stat.h"
#include "container.h"
#include "cm.h"


int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf(1,"Usage: dockv6 <mode> <args>\n");
		exit();
	}
	char * mode = argv[1];
	if (strcmp(mode, "create") == 0) {
		if (argc < 3) {
			printf(1,"Usage: dockv6 create <spec.json>\n");
			exit();
		}

		int mux_send     = mutex_get("receive");
		char * shm_send  = shm_get("ch2");

		cm_create_and_enter(argv[2]);

		mutex_lock(mux_send);
		char cmd[64];
		strcpy(cmd, mode);
		strcat(cmd, " ");
		strcat(cmd,argv[2]);
		strcpy(shm_send,cmd);
		cv_signal(mux_send);
		mutex_unlock(mux_send);

	}

	else if (strcmp(mode, "io") == 0) {
		int mux_receive  = mutex_get("send");
		int mux_send     = mutex_get("receive");

		char * shm_send  = shm_get("ch2");
		char * shm_recv  = shm_get("ch1");

		if (io_container() <= 0) exit();
		mutex_lock(mux_send);
		strcpy(shm_send,"io");
		cv_signal(mux_send);
		mutex_unlock(mux_send);


		char out[512];
		mutex_lock(mux_receive);
		cv_wait(mux_receive);
		strcpy(out, shm_recv);
		printf(1,"out: %s",out);
		mutex_unlock(mux_receive);

	}
	exit();
}