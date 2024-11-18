#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <semaphore.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h>

#define MQ_MESSAGE_SIZE 12
#define MQ_SIZE 20

#define COMM_AREA_LEN 11
#define X86_KEYBOARD_INTERRUPT 1
#define KEY_BURST_NUM 3
#define TIME_BURST_NUM 5

int key_int_id, timer_int_id;
struct sigevent keyevent, timeevent;
volatile int kcounter, tcounter;
char key_comm_ptr[COMM_AREA_LEN];
char time_comm_ptr[COMM_AREA_LEN];
const char *kmsg = " ";
const char *tmsg = " ";

mqd_t MQ_key, MQ_time;
struct mq_attr *mqstat;
char *MQ_msg_ptr;
char MQK_NAME[] = "kmq";
char MQT_NAME[] = "tmq";
int lost_kmsg_count, lost_tmsg_count;

sem_t *sem;
char SEM_NAME[] = "mysem";

struct termios org_opts, new_opts;
intrspin_t *klock;
intrspin_t *tlock;
extern void sig_handler();

const struct sigevent *key_handler(void *area, int id) {
	++kcounter;
	if ((kcounter % 2) == 0) {
		InterruptLock( klock );
		char buffer[MQ_MESSAGE_SIZE];
		itoa(kcounter/2, buffer, 10);
		strcat(buffer, kmsg);
		memcpy(area, buffer, strlen(buffer) + 1);
		InterruptUnlock(klock);
		return (&keyevent);
	} else
		return NULL;
}

const struct sigevent *time_handler(void *area, int id) {
	++tcounter;
	if ((tcounter % 1000) == 0) {
		InterruptLock( tlock );
		char buffer[MQ_MESSAGE_SIZE];
		itoa(tcounter / 1000, buffer, 10);
		strcat(buffer, tmsg);
		memcpy(area, buffer, strlen(buffer) + 1);
		InterruptUnlock(tlock);
		return (&timeevent);
	} else
		return NULL;
}

void attach_signal_handler() {
	struct sigaction act;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);

	act.sa_flags = 0;
	act.sa_mask = set;
	act.sa_handler = &sig_handler;
	sigaction(SIGINT, &act, NULL);
}

void sig_handler() {
	//------  restore old settings ---------
	tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
	InterruptUnlock( klock );
	InterruptUnlock( tlock );
	InterruptDetach(key_int_id);
	InterruptDetach(timer_int_id);
	fprintf(stderr,"ISRs detached in signal handler\n");
	fprintf(stderr,"Num of keyboard msgs lost: %d\n", lost_kmsg_count);
	fprintf(stderr,"Num of timer msgs lost: %d\n",lost_tmsg_count);
	mq_close(MQ_key);
	mq_close(MQ_time);
	mq_unlink(MQK_NAME);
	mq_unlink(MQT_NAME);
	kill(getpid(), SIGKILL);
}

void setupTerminal() {
	if (tcgetattr(STDIN_FILENO, &org_opts) != 0) return;

	//---- set new terminal parms --------
	memcpy(&new_opts, &org_opts, sizeof(new_opts));
	new_opts.c_lflag &= ~(ICANON | ECHO);
	//new_opts.c_cc[VMIN] = 0;
	//new_opts.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
}

void * key_int_task(void *arg) {
	int mq_return, counter=0;
	// this thread is dedicated to handling interrupts
	// Request I/O privileges
	ThreadCtl(_NTO_TCTL_IO, 0);

	// Attach ISR vector
	key_int_id = InterruptAttach(X86_KEYBOARD_INTERRUPT, 
			&key_handler, &key_comm_ptr, COMM_AREA_LEN, 0);

	while (1) {
		// the thread that attached the interrupt is the one that
		//must wait for the SIGEV_INTR.
		InterruptWait(0, NULL);
		char key_pressed[2] = "\0\0";
		key_pressed[0] = getchar();
		if ((key_pressed[0] > 0x20) && (key_pressed[0] < 0x7F)) {
			counter++;
			char *buffer = (char *) malloc(COMM_AREA_LEN + 4);
			memset(buffer, 0, COMM_AREA_LEN + 4);
			InterruptLock( klock );
			strcpy(buffer, "(");
			strcat(buffer, key_comm_ptr);
			strcat(buffer, ",");
			strcat(buffer, key_pressed);
			strcat(buffer, ")");
			mq_return = mq_send(MQ_key, buffer, strlen(buffer), 0);
			if (mq_return<0) lost_kmsg_count++;
			InterruptUnlock( klock );
			free(buffer);
			if (counter%KEY_BURST_NUM == 0)	sem_post(sem);
		}
	}
	return 0;
}
void * clock_int_task(void *arg) {
	int mq_return, counter=0;
	// this thread is dedicated to handling interrupts
	ThreadCtl(_NTO_TCTL_IO, 0);
	key_int_id = InterruptAttach(SYSPAGE_ENTRY(qtime)->intr, 
			&time_handler, &time_comm_ptr, COMM_AREA_LEN, 0);
	while (1) {
		InterruptWait(0, NULL);
		counter++;
		char *buffer = (char *) malloc(COMM_AREA_LEN + 1);
		memset(buffer, 0, COMM_AREA_LEN + 4);
		InterruptLock( tlock );
		strcpy(buffer, "(");
		strcat(buffer, time_comm_ptr);
		strcat(buffer, ",*)");
		mq_return = mq_send(MQ_time, buffer, strlen(buffer), 0);
		if (mq_return<0) lost_tmsg_count++;
		InterruptUnlock( tlock );
		free(buffer);
		if (counter%TIME_BURST_NUM == 0)	sem_post(sem);
	}
	return 0;
}

void processMsg(char *msg, char *qmsg) {
	strcat(msg, qmsg);
}

void process_MQ(mqd_t mqt, char *msg) {
	if (mq_getattr(mqt, mqstat) == -1) return;
	long nmsg = mqstat-> mq_curmsgs;
	if (nmsg == 0) return;

	//caller won't block if queue is full or empty.
	mqstat->mq_flags |= O_NONBLOCK;
	mq_setattr(mqt, mqstat, NULL);
	while (mq_receive(mqt,MQ_msg_ptr,MQ_MESSAGE_SIZE,NULL)> 0) {
		//a message received
		processMsg(msg, MQ_msg_ptr);
		memset(MQ_msg_ptr, 0, MQ_MESSAGE_SIZE + 1);
	}
}
void * msg_processing_task(void *arg) {
	// this thread is dedicated to processing msgs
	while (1) {
		sem_wait(sem);
		// got a signal from ISR; data ready
		char *msg1 = malloc(MQ_MESSAGE_SIZE*KEY_BURST_NUM);
		memset(msg1, 0, MQ_MESSAGE_SIZE*KEY_BURST_NUM);
		char *msg2 = malloc(MQ_MESSAGE_SIZE*TIME_BURST_NUM);
		memset(msg2, 0, MQ_MESSAGE_SIZE*TIME_BURST_NUM);
		process_MQ(MQ_key, msg1);
		process_MQ(MQ_time, msg2);
		printf("%s\n", msg1); free(msg1);
		printf("\t\t\t\t%s\n", msg2);free(msg2);
	}
	return 0;
}

static mqd_t open_MQ(char *name) {
	mqd_t queue;
	struct mq_attr attr;

	memset((void*) &attr, 0, sizeof(struct mq_attr));
	attr.mq_maxmsg = MQ_SIZE;
	attr.mq_msgsize = MQ_MESSAGE_SIZE;

	queue = mq_open(name, O_RDWR|O_CREAT|O_NONBLOCK, S_IRUSR|S_IWUSR, &attr);
	if (queue == -1)
		printf("Open queue fails %s(%d)\n", strerror(errno), errno);

	return queue;
}
void new_thread(void* (*start_routine)(void*), int priority) {
	pthread_attr_t _t_attr;
	struct sched_param schd_parameter;
	//initializes the thread attributes to their default values
	pthread_attr_init(&_t_attr);
	pthread_attr_setinheritsched(&_t_attr, PTHREAD_EXPLICIT_SCHED);

	//Set the priority
	schd_parameter.sched_priority = priority;
	pthread_attr_setschedparam(&_t_attr, &schd_parameter);
	// start up a thread
	pthread_create(NULL, &_t_attr, start_routine, NULL);
}
void initialization() {
	setupTerminal();
	attach_signal_handler();
	memset(&key_comm_ptr, 0, COMM_AREA_LEN);
	memset(&time_comm_ptr, 0, COMM_AREA_LEN);
	// Initialize event structure
	keyevent.sigev_notify = SIGEV_INTR;
	timeevent.sigev_notify = SIGEV_INTR;

	//Create a semaphore
	sem = sem_open(SEM_NAME, O_CREAT, 0644, 0);
	if (sem == SEM_FAILED) {
		perror("reader:unable to open semaphore");
		sem_unlink(SEM_NAME);
		exit(1);
	}

	MQ_key = open_MQ(MQK_NAME);
	MQ_time = open_MQ(MQT_NAME);
	mqstat = (struct mq_attr*) malloc(sizeof(struct mq_attr));
	memset(mqstat, 0, sizeof(struct mq_attr));
	MQ_msg_ptr = (char *) malloc(MQ_MESSAGE_SIZE + 1);

	klock = (intrspin_t*) malloc(sizeof(intrspin_t));
	memset(klock, 0, sizeof(*klock));
	tlock = (intrspin_t*) malloc(sizeof(intrspin_t));
	memset(tlock, 0, sizeof(*tlock));

	printf("-------------------\t\t---------------------------\n");
	printf("     Key Burst \t\t\t\t Timer Burst\n");
	printf("-------------------\t\t---------------------------\n");
}
int main() {
	ThreadCtl(_NTO_TCTL_IO, 0);
	initialization();
	new_thread(key_int_task, 60);
	new_thread(clock_int_task, 60);
	new_thread(msg_processing_task, 30);
	while (1) sleep(10);
}
