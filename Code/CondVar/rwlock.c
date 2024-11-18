// '@': a task is started, waitig for mutex
// '.': a task is waiting for read condition variable
// '*': a task is waiting for write condition variable
// 'r': a task is reading
// 'w': a task is writing
// '!': a task is done
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define THREAD_PLAYER 8

typedef struct {
	pthread_mutex_t		guard_mutex;
	pthread_cond_t		read_condvar;
	pthread_cond_t		write_condvar;
	int					rw_count; //-1 indicates a writer is active
	int					waiting_reader_count;
} rwlock_t;

rwlock_t *rwlock;
char msg[] = "    \t    \t    \t    \t    \t    \t    \t    \n";
int index[] = { 0, 0, 1, 6, 11, 16, 21, 26, 31, 36 };

void acquire_write_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	while (rwlock->rw_count != 0) {
		print_msg('*');
		pthread_cond_wait (&rwlock->write_condvar, &rwlock->guard_mutex);
	}
	rwlock->rw_count = -1;
	pthread_mutex_unlock (&rwlock->guard_mutex);
}

void release_write_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	rwlock->rw_count = 0;
	int i;
	if (rwlock->waiting_reader_count) {
		for (i = rwlock->waiting_reader_count; i>0; i--)
			pthread_cond_signal (&rwlock->read_condvar);
	} else
		//writers has lower priority than readers
		pthread_cond_signal (&rwlock->write_condvar);
	print_msg('!');
	pthread_mutex_unlock (&rwlock->guard_mutex);
}

void acquire_read_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	rwlock->waiting_reader_count ++;
	while (rwlock->rw_count < 0) {
		print_msg('.');
		pthread_cond_wait (&rwlock->read_condvar, &rwlock->guard_mutex);
	}
	rwlock->waiting_reader_count --;
	rwlock->rw_count ++;
	pthread_mutex_unlock (&rwlock->guard_mutex);
}

void release_read_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	rwlock->rw_count --;
	if (rwlock->rw_count == 0)
		//the last reader sends signal to a waiting writer
		pthread_cond_signal (&rwlock->write_condvar);
	print_msg('!');
	pthread_mutex_unlock (&rwlock->guard_mutex);
}

void print_msg(char c) {
	int myid = pthread_self();
	msg[index[myid]] = c;
	fprintf(stdout, "%s", &msg);
}

void * reader() {
	int r = rand() % 5;
	sleep(r+1); // simulate task processing
	print_msg('@');
	acquire_read_privilege(rwlock);
	print_msg('r');
	sleep(r); // simulate task processing
	release_read_privilege(rwlock);
	print_msg(' ');
}

void * writer() {
	int r = rand() % 5;
	sleep(r+1); // simulate task processing
	print_msg('@');
	acquire_write_privilege(rwlock);
	print_msg('w');
	sleep(r); // simulate task processing
	release_write_privilege(rwlock);
	print_msg(' ');
}

rwlock_t * initialize_rwlock() {
	rwlock_t *mylock = (rwlock_t *) malloc(sizeof(rwlock_t));
	memset(mylock, 0, sizeof(rwlock_t));

	pthread_mutex_init(&mylock->guard_mutex, NULL);
	pthread_cond_init(&mylock->read_condvar, NULL);
	pthread_cond_init(&mylock->write_condvar, NULL);
	mylock->rw_count = 0;
	mylock->waiting_reader_count = 0;
	return mylock;
}

int main() {
	pthread_t thread[THREAD_PLAYER];
	rwlock = initialize_rwlock();

	pthread_create(&thread[0], NULL, reader, NULL);
	pthread_create(&thread[1], NULL, reader, NULL);
	pthread_create(&thread[2], NULL, writer, NULL);
	pthread_create(&thread[3], NULL, writer, NULL);
	pthread_create(&thread[4], NULL, reader, NULL);
	pthread_create(&thread[5], NULL, writer, NULL);
	pthread_create(&thread[6], NULL, reader, NULL);
	pthread_create(&thread[7], NULL, reader, NULL);

	fprintf(stdout, " T%d\t T%d\t T%d\t T%d\t T%d\t T%d\t T%d\t T%d\n",
			thread[0], thread[1], thread[2], thread[3],
			thread[4], thread[5], thread[6], thread[7]);
	fprintf(stdout, "____\t____\t____\t____\t____\t____\t____\t____\n");
	// let the threads run for a bit
	sleep(30);
	return 1;
}

