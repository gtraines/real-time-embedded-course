//semserver.c
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SHMSZ 27
char SEM_NAME[] = "MySem";
char SHM_NAME[] = "MyShm";

int main() {
	char ch;
	int shmid;
	char *shm, *s;
	sem_t *sem;

	//sem_unlink(SEM_NAME);
	//create & initialize semaphore
	sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
	if (sem == SEM_FAILED) {
		perror("unable to create semaphore");
		sem_unlink(SEM_NAME);
		exit(EXIT_FAILURE);
	}

	if ((shmid = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRWXU)) < 0) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}
	if (ftruncate(shmid, SHMSZ) < 0) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
	//map the shared memory object to the process memory and return a pointer to it
	if ((shm = mmap(NULL, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0))
			== MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	//start writing into memory
	s = shm;
	int r;
	for (ch = 'A'; ch <= 'Z'; ch++) {
		sem_wait(sem);
		*s++ = ch;
		r = rand() % 3;
		sleep(r);
		sem_post(sem);
	}
	while (*shm != '*') {
		sleep(1);
	}
	sem_close(sem);
	sem_unlink(SEM_NAME);
	shm_unlink(SHM_NAME);
	exit(0);
}
