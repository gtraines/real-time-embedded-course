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

sem_t *sem;
char SEM_NAME[] = "mysem";

struct termios org_opts, new_opts;
intrspin_t *spinlock;
extern void sig_handler();

const struct sigevent *key_handler(void *area, int id) {
	++kcounter;
	if ((kcounter % 2) == 0) {
		InterruptLock( spinlock );
		char buffer[MQ_MESSAGE_SIZE];
		itoa(kcounter/2, buffer, 10);
		strcat(buffer, kmsg);
		memcpy(area, buffer, strlen(buffer) + 1);
		InterruptUnlock(spinlock);
		return (&keyevent);
	} else
		return NULL;
}

const struct sigevent *time_handler(void *area, int id) {
	++tcounter;
	if ((tcounter % 1000) == 0) {
		InterruptLock( spinlock );
		char buffer[MQ_MESSAGE_SIZE];
		itoa(tcounter / 1000, buffer, 10);
		strcat(buffer, tmsg);
		memcpy(area, buffer, strlen(buffer) + 1);
		InterruptUnlock(spinlock);
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
	InterruptUnlock( spinlock );
	InterruptDetach(key_int_id);
	InterruptDetach(timer_int_id);
	fprintf(stderr, "ISRs detached in signal handler\n");
	sem_close(sem);
	sem_unlink(SEM_NAME);
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

void * key_int_thread(void *arg) {
	int counter=0;
	// this thread is dedicated to handling interrupts
	// Request I/O privileges
	ThreadCtl(_NTO_TCTL_IO, 0);

	// Attach ISR vector
	key_int_id = InterruptAttach(X86_KEYBOARD_INTERRUPT, &key_handler,
			&key_comm_ptr, COMM_AREA_LEN, 0);

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
			strcpy(buffer, "(");
			strcat(buffer, key_comm_ptr);
			strcat(buffer, ",");
			strcat(buffer, key_pressed);
			strcat(buffer, ")");
			mq_send(MQ_key, buffer, strlen(buffer), 0);
			free(buffer);
			if (counter%KEY_BURST_NUM == 0)	sem_post(sem);
		}
	}
	return 0;
}
void * time_int_thread(void *arg) {
	int counter=0;
	// this thread is dedicated to handling interrupts
	ThreadCtl(_NTO_TCTL_IO, 0);
	key_int_id = InterruptAttach(SYSPAGE_ENTRY(qtime)->intr, &time_handler,
			&time_comm_ptr, COMM_AREA_LEN, 0);
	while (1) {
		InterruptWait(0, NULL);
		counter++;
		char *buffer = (char *) malloc(COMM_AREA_LEN + 1);
		memset(buffer, 0, COMM_AREA_LEN + 4);
		strcpy(buffer, "(");
		strcat(buffer, time_comm_ptr);
		strcat(buffer, ",*)");
		mq_send(MQ_time, buffer, strlen(buffer), 0);
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
	while (mq_receive(mqt, MQ_msg_ptr, MQ_MESSAGE_SIZE, NULL) > 0) {
		//a message received
		InterruptUnlock( spinlock );
		processMsg(msg, MQ_msg_ptr);
		memset(MQ_msg_ptr, 0, MQ_MESSAGE_SIZE + 1);
		InterruptLock( spinlock );
	}
}
void * task_thread(void *arg) {
	// this thread is dedicated to processing msgs
	while (1) {
		sem_wait(sem);
		// got a signal from ISR; data ready
		char *msg1 = malloc(MQ_MESSAGE_SIZE*KEY_BURST_NUM);
		memset(msg1, 0, MQ_MESSAGE_SIZE*KEY_BURST_NUM);
		char *msg2 = malloc(MQ_MESSAGE_SIZE*TIME_BURST_NUM);
		memset(msg2, 0, MQ_MESSAGE_SIZE*TIME_BURST_NUM);
		InterruptLock( spinlock );
		process_MQ(MQ_key, msg1);
		process_MQ(MQ_time, msg2);
		InterruptUnlock( spinlock );
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

	queue = mq_open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr);
	if (queue == -1)
		printf("Couldn't open queue error is %s(%d)\n", strerror(errno), errno);

	return queue;
}
void new_thread(void* (*start_routine)(void*), int priority) {
	pthread_attr_t _thread_attr;
	struct sched_param schd_parameter;
	//initializes the thread attributes to their default values
	pthread_attr_init(&_thread_attr);
	pthread_attr_setinheritsched(&_thread_attr, PTHREAD_EXPLICIT_SCHED);

	//Set the priority
	schd_parameter.sched_priority = priority;
	pthread_attr_setschedparam(&_thread_attr, &schd_parameter);
	// start up a thread
	pthread_create(NULL, &_thread_attr, start_routine, NULL);
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

	spinlock = (intrspin_t*) malloc(sizeof(intrspin_t));
	memset(spinlock, 0, sizeof(*spinlock));
	printf("-------------------\t\t------------------------------\n");
	printf("     Key Burst \t\t\t\t Timer Burst\n");
	printf("-------------------\t\t------------------------------\n");
}
int main() {
	ThreadCtl(_NTO_TCTL_IO, 0);
	initialization();
	new_thread(key_int_thread, 10);
	new_thread(time_int_thread, 10);
	new_thread(task_thread, 10);
	while (1) sleep(10);
}
