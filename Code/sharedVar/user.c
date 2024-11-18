// qcc -o user user.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char **argv) {
	int fd, i, nloop = 3, *ptr;

	//open a file and map it into memory
	fd = open("log.txt", O_RDWR, S_IRWXU);
	ptr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	for (i = 0; i < nloop; i++) {
		printf("User %d starts processing data: %d\n", getpid(),
				(*ptr)++);
		sleep(2); //simulate processing
		printf("User %d stops processing data\n", getpid());
		sleep(1);
	}
	exit(0);
}
