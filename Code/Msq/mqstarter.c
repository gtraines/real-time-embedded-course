#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <mqueue.h>
#include "myheader.h"

static void create_msg_queue(char *name, int msg_len) {
	struct mq_attr attr;
	mqd_t queue;

	mq_unlink(name);
	memset((void*) &attr, 0, sizeof(struct mq_attr));
	attr.mq_maxmsg = MQ_CAPACITY;
	attr.mq_msgsize = msg_len;
	queue = mq_open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr);
	if (queue == -1)
		printf("Couldn't open queue: %s(%d)\n", strerror(errno), errno);
	mq_close(queue);
}

int main(int argc, char **argv) {
	char *args0[] = { "mqwriter", "0", NULL };
	char *args1[] = { "mqwriter", "1", NULL };
	char *args2[] = { "mqreader", "2", NULL };
	struct inheritance inherit;
	pid_t pid, pidwa, pidwb, pidr;
	int status;

	//Creating Message Queue
	create_msg_queue(MQ_NAME, sizeof(myMSG_t));

	pmsg[59] = '\n';
	memset(pmsg, ASCII_SPACE, PRT_STR_LEN-1);
	memcpy(&pmsg[prt_index[0]], "Writer A", 8);
	memcpy(&pmsg[prt_index[1]], "Writer B", 8);
	memcpy(&pmsg[prt_index[2]], "Reader", 6);
	memcpy(&pmsg[prt_index[3]], "MQ Status", 9);
	fprintf(stdout, "%s", pmsg);
	memset(pmsg, Hyphen, PRT_STR_LEN-1);
	fprintf(stdout, "%s", pmsg);

	// create 3 child processes
	inherit.flags = 0;
	pidwa = spawn(args0[0], 0, NULL, &inherit, args0, environ);
	pidwb = spawn(args1[0], 0, NULL, &inherit, args1, environ);
	pidr =  spawn(args2[0], 0, NULL, &inherit, args2, environ);
	// spawn() is QNX function. Use posix_spawn() for portability
	// posix_spawn(&pidwa, args0[0], NULL, NULL, args0, environ);
	// posix_spawn(&pidwb, args1[0], NULL, NULL, args1, environ);
	// posix_spawn(&pidr,  args2[0], NULL, NULL, args2, environ);

	while (1) {
		if ((pid = wait(&status)) == -1) {
			perror("Starter: ");
			mq_unlink(MQ_NAME);
			exit(EXIT_FAILURE);
		}
		printf("User process %d terminated\n", pid);
	}
}
