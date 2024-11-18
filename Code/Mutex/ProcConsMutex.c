#include <stdio.h>
#include <pthread.h>
#include <time.h>

//Data is shared by children processes producer and consumer
int shared_buffer = -1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void set_shared_buffer(int value) {
	fprintf(stdout, "Producer writes\t%2d", value);
	shared_buffer = value;
}
int get_shared_buffer() {
	fprintf(stdout, "Consumer reads\t%2d\n", shared_buffer);
	return shared_buffer;
}
int get_shared_buffer2() {
	return shared_buffer;
}

void * consumer(void *notused) {
	int sum = 0;
	int firstget, secondget;
	int r, count;
	for (count = 1; count <= 10; count++) {
		r = rand() % 4;
		sleep(r);

		pthread_mutex_lock(&mutex);
		//start of critical section
		firstget = get_shared_buffer();
		sleep(1); //simulate longer processing
		secondget = get_shared_buffer2();
		sum += secondget;
		fprintf(stdout, "Consumer process\t\t\t%2d,%2d,%2d\n", 
			firstget, secondget, sum);
		//end of critical section
		pthread_mutex_unlock(&mutex);
	}
	fprintf(stdout, "\n%s %d\n%s\n", "Consumer read values total", sum, "Terminating Consumer");
}

void * producer(void *notused) {
	int sum = 0;
	int r, count;
	for (count = 1; count <= 10; count++) {
		r = rand() % 4;
		sleep(r);

		pthread_mutex_lock(&mutex);
		//start of critical section
		set_shared_buffer(count);
		sum += count;
		fprintf(stdout, "\t%2d\n", sum);
		//end of critical section
		pthread_mutex_unlock(&mutex);
	}
	fprintf(stdout, "Producer done\nTerminating Producer\n");
}

int main() {
	fprintf(stdout, "Action\t\tValue\tSum of Produced\tfirst, second,Sum\n");
	fprintf(stdout, "------\t\t-----\t---------------\t---------------\n");

	pthread_attr_t attr;
	struct sched_param sp;

	//initializes the thread attributes to their default values
	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

	//Set the priority to 30 to the producer thread
	sp.sched_priority = 30;
	pthread_attr_setschedparam(&attr, &sp);
	// create the producer thread
	pthread_create(NULL, &attr, producer, NULL);

	//Set the priority to 20 to the consumer thread
	sp.sched_priority = 20;
	pthread_attr_setschedparam(&attr, &sp);
	// create the consumer thread
	pthread_create(NULL, &attr, consumer, NULL);

	// let the threads run for a bit
	sleep(30);
	return 1;
}

