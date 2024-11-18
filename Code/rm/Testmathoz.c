#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int Test_operator(int num1, char *op, int num2) {
	int fd1 = open("/dev/mathoz/o1", O_WRONLY);
	char * input1 = (char*)malloc(10);
	itoa(num1, input1, 10);
	write(fd1, input1, 1);
	int fd2 = open("/dev/mathoz/operator", O_WRONLY);
	write(fd2, op, 1);
	int fd3 = open("/dev/mathoz/o2", O_WRONLY);
	char * input2 = (char*)malloc(10);
	itoa(num2, input2, 10);
	write(fd3, input2, 1);
	int fd4 = open("/dev/mathoz/result", O_RDONLY);
	char *result = malloc(10);
	read(fd4, result, 10);
	close(fd1); close(fd2); close(fd3); close(fd4);
	printf ("Result: %s.\n", result);
	return 0;
}

int main(int argc, char *argv[]) {

	Test_operator(7, "+", 4);
	Test_operator(7, "-", 4);
	Test_operator(7, "X", 4);
	Test_operator(7, "%", 4);
	return EXIT_SUCCESS;
}
