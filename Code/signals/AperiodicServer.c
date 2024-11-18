#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h>

sem_t *sem;
int curvalue = 0;

int main( void )
  {
    extern void sig_preemption_handler(int, siginfo_t*, void*);
    extern void sig_resumption_handler(int, siginfo_t*, void*);
    struct sigaction act1, act2;
    sigset_t set1, set2;
    char SEM_NAME[] = "mysem";

    printf("%d\n", getpid());
    //Create a semaphore
    sem_unlink(SEM_NAME);
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 0);
    if (sem == SEM_FAILED) {
        perror("reader:unable to open semaphore");
        sem_unlink(SEM_NAME);
        exit(1);
    }

    sigemptyset( &set1 );
    sigaddset( &set1, SIGUSR1 ); //Used as Preemption Signal
    act1.sa_mask = set1;
    act1.sa_flags = SA_SIGINFO; // make it a queued signal
    act1.sa_sigaction = &sig_preemption_handler;
    sigaction( SIGUSR1, &act1, NULL );

    sigemptyset( &set2 );
    sigaddset( &set2, SIGUSR2 ); //Used as Resumption Signal
    act2.sa_mask = set2;
    act2.sa_flags = SA_SIGINFO; // make it a queued signal
    act2.sa_sigaction = &sig_resumption_handler;
    sigaction( SIGUSR2, &act2, NULL );

    //the calling process of sem_wait blocks until it can 
    //decrement the counter, or the call is interrupted by signal
    //Two consecutive calls are needed for such a reason
    sem_wait(sem); // to be interrupted by SIGUSR2 
    sem_wait(sem); //this effectively decrements semaphore to 0
    while (1){
        aperiodic_jobs();
    }
    return EXIT_SUCCESS;
  }
int aperiodic_jobs(void){
    //simulate execution of aperiodic_jobs
    double i=0;
    fprintf(stderr, "^");
    for (i=0; i<10000000; i++) {
        double a = pow(i+10, i);
    }
    fprintf(stderr, "%d", curvalue);
}
void sig_preemption_handler(int signo, siginfo_t* info, void* other)
  {
    curvalue = info->si_value.sival_int;
    fprintf(stderr, "-O");
    //make sure SIGUSR2 is not masked in initialization, 
    //otherwise SIGUSR2 cannot be nestedly handled
    sem_wait(sem); // to be interrupted by SIGUSR2 
    //now the control is returned from sig_resumption_handler
    fprintf(stderr, "K-");
    sem_wait(sem); //this effectively decrements semaphore to 0
  }
void sig_resumption_handler(int signo, siginfo_t* info, void* other)
  {
    curvalue = info->si_value.sival_int;
    fprintf(stderr, ".");
    sem_post(sem);
  }
