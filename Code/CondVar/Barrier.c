#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define THREAD_PLAYER 4

typedef struct {
	pthread_mutex_t barrier_lock;
	pthread_cond_t barrier_condvar;
	int barrier_count; //how many are ready
	int barrier_threshold; //how many to sync
} barrier_t;

barrier_t *barrier;
char msg[] ="   0  \t\t________\t________\t________\t________\n";
int index[] ={ 0, 3, 11, 21, 30, 39 };
char zero = '0';
int step = 0;

void print_msg() {
	int myid = pthread_self();
	step++;
	msg[3] = zero + (step % 10);
	msg[index[myid]] = '!';
	fprintf(stdout, "\n");
	fprintf(stdout, &msg);
}

barrier_t * initialize_barrier(int threshold) {
	barrier_t *mybarrier = (barrier_t *) malloc(sizeof(barrier_t));
	memset(mybarrier, 0, sizeof(barrier_t));

	pthread_mutex_init(&mybarrier->barrier_lock, NULL);
	pthread_cond_init(&mybarrier->barrier_condvar, NULL);
	mybarrier->barrier_threshold = threshold;
	mybarrier->barrier_count = 0;
	return mybarrier;
}

void barrier_sync_point(barrier_t *barrier) {
	pthread_mutex_lock(&barrier->barrier_lock);
	print_msg();
	barrier->barrier_count++;
	if (barrier->barrier_count < barrier->barrier_threshold) {
		pthread_cond_wait(&barrier->barrier_condvar, &barrier->barrier_lock);
	} else { //enough threads are ready
		fprintf(stdout, "\nEnough threads passed the barrier!\n");
		barrier->barrier_count = 0;
		int i;
		for (i = barrier->barrier_threshold; i > 0; i--)
			pthread_cond_signal(&barrier->barrier_condvar);
	}
	pthread_mutex_unlock(&barrier->barrier_lock);
}

void * usertask() {
	int r = rand() % 5;
	sleep(r+1); // simulate task processing
	barrier_sync_point(barrier);
	sleep(r); // simulate task processing
	fprintf(stdout, "Thread %d is done!\n", pthread_self());
}

int main() {
	pthread_t thread[THREAD_PLAYER];
	barrier = initialize_barrier(THREAD_PLAYER);
	pthread_attr_t attr;
	struct sched_param sp;
	//initializes the thread attributes to their default values
	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

	//Set the priority to 30
	sp.sched_priority = 30;
	pthread_attr_setschedparam(&attr, &sp);

	int i;
	for (i = 0; i < 4; i++) {
		pthread_create(&thread[i], &attr, usertask, NULL);
	}
	fprintf(stdout, "Step\t\tThread %d\tThread %d\tThread %d\tThread %d\n",
			thread[0], thread[1], thread[2], thread[3]);
	fprintf(stdout, &msg);
	// let the threads run for a bit
	sleep(20);
	return 1;
}

