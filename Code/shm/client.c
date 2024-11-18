//client.c
//adapted from Dave Marshall, Programming in C []
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHMSZ 27
char SHM_NAME[] = "MyShm";

int main() {
	int shmid;
	char *shm, *s;

	/* open the shared memory object */
	shmid = shm_open(SHM_NAME, O_RDWR, 0);
	if (shmid == -1) {
		perror("client: error opening the shared memory object\n");
		exit(1);
	}
	/* get a pointer to a piece of the shared memory, note that we
	 only map in the amount we need to */
	shm = mmap(0,SHMSZ,PROT_READ|PROT_WRITE, MAP_SHARED, shmid, 0);
	if (shm == MAP_FAILED) {
		perror("client: mmap failed\n");
		exit(1);
	}

	//Dirty data: the 1st read and 2nd read can be different
	int i, r;
	for (i = 0; i < 20; i++) {
		printf("1st read:-- %s\n",shm);
		r = rand() % 3;
		sleep(r);
		printf("2nd read:-- %s\n",shm);
	}

	//once done, inform the server
	*shm = '*';
	exit(0);
}
