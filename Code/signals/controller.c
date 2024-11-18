#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/siginfo.h>
#include <unistd.h>
#include <time.h>

int main( int argc, char *argv[] )
  {
    int pid, r, i=0;
    struct sigevent event1, event2;

    if (argc!=2){
    	fprintf(stderr, "Use: controller pid");
    	exit(EXIT_FAILURE);
    }
    SIGEV_SIGNAL_VALUE_INIT( &event1, SIGUSR1, 1 );
    SIGEV_SIGNAL_VALUE_INIT( &event2, SIGUSR2, 2 );
    pid = atoi(argv[1]);
    srand( time( NULL ) );

    while (1){
    	r = 1+ rand() % 8;
    	sleep (r);
    	if ((i%2)==1){
    		sigqueue(pid, SIGUSR1, event1.sigev_value );
    	}
    	else {
    		sigqueue(pid, SIGUSR2, event2.sigev_value );
    	}
    	i++;
    }
    return EXIT_SUCCESS;
  }

