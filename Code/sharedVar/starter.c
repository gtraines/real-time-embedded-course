#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
	char *args[] = { "user", NULL };
	int i, status, fd, value = 0;
	struct inheritance inherit;
	pid_t pid;

	//open a file and set initial value
	fd = open("log.txt", O_RDWR | O_CREAT, S_IRWXU);
	write(fd, &value, sizeof(int));
	close(fd);

	// create 3 child processes
	for (i = 0; i < 3; i++) {
		inherit.flags = 0;
		if ((pid = spawn("user", 0, NULL, &inherit, args, environ)) == -1)
			perror("spawn() failed");
		else
			printf("spawned child, pid = %d\n", pid);
	}
	while (1) {
		if ((pid = wait(&status)) == -1) {
			perror("Starter: ");
			exit(EXIT_FAILURE);
		}
		printf("User %d terminated\n", pid);
	}
}
