#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>

#define PRT_STR_LEN 90
#define ASCII_SPACE 32
#define Hyphen 45
struct pipe_t {
	int rfd;
	int wfd;
};
struct pipe_t mypipe[4];
fd_set a_rfd, a_wfd, b_rfd, b_wfd;
char ch = 'A', num = '0';
char pmsg[PRT_STR_LEN];
int prt_index[] = { 1, 15, 30, 45, 60, 75 };

void reset_FD_SETS(int cpid) {
	int i;
	if (cpid == 0) { //child process B
		FD_ZERO( &b_rfd );
		FD_ZERO( &b_wfd );
		for (i = 0; i < 4; i++) {
			if (i < 2)
				FD_SET( mypipe[i].rfd, &b_rfd );
			else
				FD_SET( mypipe[i].wfd, &b_wfd );
		}
	} else { //parent process A
		FD_ZERO( &a_rfd );
		FD_ZERO( &a_wfd );
		for (i = 0; i < 4; i++) {
			if (i < 2)
				FD_SET( mypipe[i].wfd, &a_wfd );
			else
				FD_SET( mypipe[i].rfd, &a_rfd );
		}
	}
	memset(pmsg, Hyphen, PRT_STR_LEN-1);
	fprintf(stdout, "%s", pmsg);
}
char *pipe_right(char ch) {
	char *str = (char *) malloc(4);
	memset(str, 0, 4);
	strcpy(str, ">");
	memset(str + 1, ch, 1);
	strcat(str, ">");
	return str;
}
char *pipe_left(char ch) {
	char *str = (char *) malloc(4);
	memset(str, 0, 4);
	strcpy(str, "<");
	memset(str + 1, ch, 1);
	strcat(str, "<");
	return str;
}

void writechar(int mindex, int pindex) {
	int result;
	result = write(mypipe[pindex].wfd, &ch, 1);
	if (result != 1) {
		perror("write");
		exit(2);
	}
	if (mindex == 0)
		print_msg(mindex, pindex + 1, "Write", pipe_right(ch));
	else
		print_msg(mindex, pindex + 1, "Write", pipe_left(ch));
	if (ch == 'J') ch = 'A' - 1;
	ch++;
}

void writenum(int mindex, int pindex) {
	int result;
	result = write(mypipe[pindex].wfd, &num, 1);
	if (result != 1) {
		perror("write");
		exit(2);
	}
	if (mindex == 0)
		print_msg(mindex, pindex + 1, "Write", pipe_right(num));
	else
		print_msg(mindex, pindex + 1, "Write", pipe_left(num));
	if (num == '9') num = '0' - 1;
	num++;
}

void readchar(int mindex, int pindex) {
	char ch;
	int result;
	result = read(mypipe[pindex].rfd, &ch, 1);
	if (result != 1) {
		perror("read");
		exit(3);
	}
	if (mindex == 0)
		print_msg(mindex, pindex + 1, "Read", pipe_left(ch));
	else
		print_msg(mindex, pindex + 1, "Read", pipe_right(ch));
}
void print_msg(int myindex, int pindex, char *msg1, char *msg2) {
	memset(pmsg, ASCII_SPACE, PRT_STR_LEN - 1);
	pmsg[PRT_STR_LEN - 1] = '\n';
	memcpy(&pmsg[prt_index[myindex]], msg1, strlen(msg1));
	memcpy(&pmsg[prt_index[pindex]], msg2, strlen(msg2));
	fprintf(stdout, "%s", pmsg);
}
void print_header(){
	memset(pmsg, ASCII_SPACE, PRT_STR_LEN-1);
	memcpy(&pmsg[prt_index[0]], "Process A", 9);
	memcpy(&pmsg[prt_index[1]], "Pipe 1", 6);
	memcpy(&pmsg[prt_index[2]], "Pipe 2", 6);
	memcpy(&pmsg[prt_index[3]], "Pipe 3", 6);
	memcpy(&pmsg[prt_index[4]], "Pipe 4", 6);
	memcpy(&pmsg[prt_index[5]], "Process B", 9);
	fprintf(stdout, "%s", pmsg);
}
int main(int argc, char **argv) {
	int i, n, result, cpid, maxfd=0;
	int fd[2];
	pmsg[PRT_STR_LEN - 1] = '\n';

	/* create the pipes */
	for (i = 0; i < 4; i++) {
		result = pipe(fd);
		if (result < 0) {
			perror("pipe ");
			exit(1);
		}
		mypipe[i].rfd = fd[0];
		mypipe[i].wfd = fd[1];
		if (mypipe[i].rfd > maxfd)
			maxfd = mypipe[i].rfd;
		if (mypipe[i].wfd > maxfd)
			maxfd = mypipe[i].wfd;
	}
	//fork() returns 0 to the child (new) process,
	//and returns the PID of the child to the parent process.
	cpid = fork();
	if (cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (cpid == 0) { //this is the child process B
		sleep(1);
		reset_FD_SETS(cpid);
		while ((n = select(maxfd + 1, &b_rfd, &b_wfd, 0, 0)) > 0) {
			if (FD_ISSET( mypipe[0].rfd, &b_rfd ))
				readchar(5, 0);
			if (FD_ISSET( mypipe[1].rfd, &b_rfd ))
				readchar(5, 1);
			if (FD_ISSET( mypipe[2].wfd, &b_wfd ))
				writenum(5, 2);
			if (FD_ISSET( mypipe[3].wfd, &b_wfd ))
				writenum(5, 3);
			reset_FD_SETS(cpid);
			sleep(2);
		}
		_exit(EXIT_SUCCESS);
	} else { //this is the parent process A
		reset_FD_SETS(cpid);
		print_header();
		sleep(2);
		while ((n = select(maxfd + 1, &a_rfd, &a_wfd, 0, 0)) > 0) {
			if (FD_ISSET( mypipe[0].wfd, &a_wfd ))
				writechar(0, 0);
			if (FD_ISSET( mypipe[1].wfd, &a_wfd ))
				writechar(0, 1);
			if (FD_ISSET( mypipe[2].rfd, &a_rfd ))
				readchar(0, 2);
			if (FD_ISSET( mypipe[3].rfd, &a_rfd ))
				readchar(0, 3);
			reset_FD_SETS(cpid);
			sleep(2);
		}
		wait(NULL); /* Wait for child */
		exit(EXIT_SUCCESS);
	}
}
