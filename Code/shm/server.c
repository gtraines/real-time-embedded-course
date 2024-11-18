//server.c
//adapted from Dave Marshall, Programming in C []
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#define SHMSZ 27
char SHM_NAME[] = "MyShm";

int main() {
	char ch;
	int shmid;
	char *shm, *s;

	if ((shmid = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRWXU))<0) {
		perror("shm_open");
		return EXIT_FAILURE;
	}

	/* Set the size of the shared memory object */
	if( ftruncate( shmid, SHMSZ ) == -1 ) {
		perror("out of memory");
		return EXIT_FAILURE;
	}
	//map the shared memory object to the process memory
	if ((shm = mmap(NULL, SHMSZ, PROT_READ|PROT_WRITE, MAP_SHARED,
			shmid, 0)) == MAP_FAILED) {
		perror("memory mapping");
		return EXIT_FAILURE;
	}

	//start writing into memory
	s = shm;
	int r;
	for (ch = 'A'; ch <= 'Z'; ch++) {
		*s++ = ch;
		r = rand() % 3;
		sleep(r);
	}

	while (*shm != '*') sleep(1);
	shm_unlink(SHM_NAME);
	return EXIT_SUCCESS;
}
