#define SHM_MAXNUM 16


struct shm_info{
	char *page_addr;
	char name[150];
	int ref_count;
};