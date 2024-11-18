#include <semaphore.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char **argv) {
	int fd, i, nloop = 3, value = 0, *ptr;
	sem_t *sem;
	int shm;

	//open a file and map it into memory
	fd = open("log.txt", O_RDWR | O_CREAT, S_IRWXU);
	write(fd, &value, sizeof(int));
	ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	//create a shared memory region of size sem_t
	// shm_open takes a name for the shared memory region,
	// Other processes just need to know and use the same name.
	if ((shm = shm_open("myshm", O_RDWR | O_CREAT, S_IRWXU)) < 0) {
		perror("shm_open");
		exit(1);
	}
	if (ftruncate(shm, sizeof(sem_t)) < 0) {
		perror("ftruncate");
		exit(1);
	}

	//map the shared memory object to the process memory and return a pointer to it
	if ((sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED,
			shm, 0)) == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	/* create, initialize an unnamed semaphore */
	if (sem_init(sem, 1, 1) < 0) {
		perror("semaphore initialization");
		exit(0);
	}
	if (fork() == 0) { /* child process*/
		for (i = 0; i < nloop; i++) {
			sem_wait(sem);
			printf("child entered critical section: %d\n", (*ptr)++);
			sleep(2);
			printf("child leaving critical section\n");
			sem_post(sem);
			sleep(1);
		}
		sem_close(sem);
		exit(0);
	}
	/* back to parent process */
	for (i = 0; i < nloop; i++) {
		sem_wait(sem);
		printf("parent entered critical section: %d\n", (*ptr)++);
		sleep(2);
		printf("parent leaving critical section\n");
		sem_post(sem);
		sleep(1);
	}
	sem_destroy(sem);
	exit(0);
}
