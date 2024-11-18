#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/siginfo.h>
#include <unistd.h>
#include <time.h>

int main( int argc, char *argv[] )
  {
    int pid1, pid2, r;
    struct sigevent event1, event2;

    if (argc!=3){
    	fprintf(stderr, "Use: signaler pid1 pid2");
    	exit(EXIT_FAILURE);
    }
    SIGEV_SIGNAL_VALUE_INIT( &event1, SIGUSR1, 1 );
    SIGEV_SIGNAL_VALUE_INIT( &event2, SIGUSR1, 0 );
    pid1 = atoi(argv[1]);
    pid2 = atoi(argv[2]);
    srand( time( NULL ) );

    do {
			r = rand() % 3;
			if (r == 2){
				fprintf(stderr, "*");

				//send a signal
				//kill( pid1, SIGUSR1 );
				//kill( pid2, SIGUSR1 );
				sigqueue(pid1, SIGUSR1, event1.sigev_value );
				sigqueue(pid2, SIGUSR1, event2.sigev_value );
			}
			sleep (1);
    } while( r != 3 ); /* end do...while */

    return EXIT_SUCCESS;
  }

