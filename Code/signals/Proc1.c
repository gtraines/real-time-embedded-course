#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int main( void )
  {
    extern void handler(int, siginfo_t*, void*);
    struct sigaction act;
    sigset_t set;


    printf("%d\n", getpid());
    /*
     * Define a handler for SIGUSR1.
     */
    sigemptyset( &set );
    sigaddset( &set, SIGUSR1 );
    act.sa_mask = set;
    act.sa_sigaction = &handler;
    act.sa_flags = SA_SIGINFO; // make it a queued signal
    sigaction( SIGUSR1, &act, NULL );

	while (1){
		fprintf(stderr, ".");
		sleep (1);
		fprintf(stderr, ".");
		sleep (1);
		fprintf(stderr, ".");
		sleep (1);
	}
    return EXIT_SUCCESS;
  }

void handler( int signo, siginfo_t* info, void* other )
  {
    fprintf(stderr, "%d", info->si_value.sival_int);
  }
