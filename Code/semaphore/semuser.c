// semuser.c
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SHMSZ 27
char SEM_NAME[] = "MySem";
char SHM_NAME[] = "MyShm";

int main() {
	int shmid;
	char *shm, *s;
	sem_t *sem;

	//create & initialize existing semaphore
	sem = sem_open(SEM_NAME, 0, 0644, 0);
	if (sem == SEM_FAILED) {
		perror("reader:unable to execute semaphore");
		sem_close(sem);
		exit(EXIT_FAILURE);
	}

	/* open the shared memory object */
	shmid = shm_open(SHM_NAME, O_RDWR, 0);
	if (shmid == -1) {
		perror("client: error opening the shared memory object\n");
		exit(EXIT_FAILURE);
	}
	/* get a pointer to a piece of the shared memory, note that we
	 only map in the amount we need to */
	shm = mmap(0, SHMSZ, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
	if (shm == MAP_FAILED) {
		perror("client: mmap failed\n");
		exit(EXIT_FAILURE);
	}

	//Protected: the 1st read and the 2nd read are the same
	int i, r;
	for (i = 0; i < 20; i++) {
		sem_wait(sem);
		printf("1st read:-- %s\n",shm);
		r = rand() % 3;
		sleep(r);
		printf("2nd read:-- %s\n",shm);
		sem_post(sem);
	}

	//once done, inform the server
	*shm = '*';
	sem_close(sem);
	exit(0);
}
