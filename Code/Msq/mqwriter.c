#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include "myheader.h"

void send_msg(char *sender, int sequence, char *msg, mqd_t _MQ){
	myMSG_t *_msg = (myMSG_t *) malloc(sizeof(myMSG_t));
	memset(_msg, 0, sizeof(myMSG_t));
	_msg->sequence = sequence;
	memcpy(_msg->content, msg, strlen(msg));
	memcpy(_msg->sender, sender, strlen(sender));
	mq_send(_MQ, _msg, sizeof(myMSG_t), 0);
	free(_msg);
}

int main(int argc, char *argv[]) {
	mqd_t myMQ;
	static int seq=0;

    if (argc!=2){
    	fprintf(stderr, "Use: mqwriter index");
    	exit(EXIT_FAILURE);
    }
    int myindex = atoi(argv[1]);
    char *sender = senders[myindex];
	pmsg[PRT_STR_LEN-1] = '\n';

	myMQ = mq_open(MQ_NAME, O_WRONLY);
	mqstat = (struct mq_attr*) malloc(sizeof(struct mq_attr));
	memset(mqstat, 0, sizeof(struct mq_attr));

	while(1){
		int r = rand() % 3 + 1;
		sleep(r); // simulate task processing

		char *msg = (char *) malloc(10);
		memset(msg, 0, 10);
		strcpy(msg, prefix[myindex]);
		itoa(++seq, msg+strlen(msg), 10);
		strcat(msg, ")");

		if (MQ_size(myMQ)==MQ_CAPACITY) //MQ is full
			print_msg(myindex, ".", full_mq);
		send_msg(sender, seq, msg, myMQ);
		long size = MQ_size(myMQ);
		print_msg(myindex, msg, mq_status(size));
	}
	mq_close(myMQ);
	return EXIT_SUCCESS;
}

