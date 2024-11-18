// qcc -o semclient semclient.c
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char **argv) {
	int fd, i, nloop = 3, *ptr;
	sem_t *sem;
	char SEM_NAME[] = "mysem";

	//open a file and map it into memory
	fd = open("log.txt", O_RDWR, S_IRWXU);
	ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	//open an existing semaphore
	sem = sem_open(SEM_NAME, 0, 0644, 0);
	if (sem == SEM_FAILED) {
		perror("reader:unable to open semaphore");
		sem_close(sem);
		exit(1);
	}

	for (i = 0; i < nloop; i++) {
		sem_wait(sem);
		printf("Semclient %d entered critical section: %d\n", getpid(),
				(*ptr)++);
		sleep(2); //simulate processing
		printf("Semclient %d leaving critical section\n", getpid());
		sem_post(sem);
		sleep(1);
	}
	sem_close(sem);
	exit(0);
}
