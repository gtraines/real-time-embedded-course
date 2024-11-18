#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
	char SEM_NAME[] = "mysem";
	char *args[] = { "semclient", NULL };
	int i, status, fd, value = 0;
	struct inheritance inherit;
	pid_t pid;
	sem_t *sem;

	//open a file and set initial value
	fd = open("log.txt", O_RDWR | O_CREAT, S_IRWXU);
	write(fd, &value, sizeof(int));
	close(fd);

	//create & initialize a semaphore
	sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
	if (sem == SEM_FAILED) {
		perror("unable to create semaphore");
		sem_unlink(SEM_NAME);
		exit(1);
	}

	// create 3 child processes
	for (i = 0; i < 3; i++) {
		inherit.flags = 0;
		if ((pid = spawn("semclient", 0, NULL, &inherit, args, environ)) == -1)
			perror("spawn() failed");
		else
			printf("spawned child, pid = %d\n", pid);
	}
	while (1) {
		if ((pid = wait(&status)) == -1) {
			perror("Starter: ");
			sem_unlink(SEM_NAME);
			exit(EXIT_FAILURE);
		}
		printf("Semclient %d terminated\n", pid);
	}
}
