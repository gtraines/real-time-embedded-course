/*
 * myheader.h
 */
#ifndef MYHEADER_H_
#define MYHEADER_H_

#define MQ_CAPACITY 5
#define PRT_STR_LEN 60
#define ASCII_SPACE 32
#define Hyphen 45

typedef struct {
	int sequence;
	char content[10];
	char sender[10];
} myMSG_t;

char *senders[2] = {"Writer A", "Writer B"};
char *prefix[2] = {"(A,", "(B,"};
char MQ_NAME[] = "mymq";
struct mq_attr *mqstat;
char pmsg[PRT_STR_LEN];
int prt_index[] = { 1, 15, 30, 45 };
char empty_mq[] = "[.....]";
char full_mq[] = "[*****]";

void print_msg(int myindex, char *msg1, char *msg2) {
	memset(pmsg, ASCII_SPACE, PRT_STR_LEN-1);
	memcpy(&pmsg[prt_index[myindex]], msg1, strlen(msg1));
	memcpy(&pmsg[prt_index[3]], msg2, strlen(msg2));
	fprintf(stdout, "%s", pmsg);
	memset(pmsg, Hyphen, PRT_STR_LEN-1);
	fprintf(stdout, "%s", pmsg);
}
long MQ_size(mqd_t _MQ){
	if (mq_getattr(_MQ, mqstat) == -1) return -1;
	long nmsg = mqstat-> mq_curmsgs;
	return nmsg;
}
char *mq_status(long len){
	char *status = (char *) malloc(MQ_CAPACITY+3);
	memset(status, 0, MQ_CAPACITY+3);
	strcpy(status, "[");
	long counter;
	for (counter=1;counter<=len;counter++) strcat(status, "*");
	for (;counter<MQ_CAPACITY+1;counter++) strcat(status, ".");
	strcat(status, "]");
	return status;
}
#endif /* MYHEADER_H_ */
