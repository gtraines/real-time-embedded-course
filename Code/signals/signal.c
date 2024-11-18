#include <signal.h>
#include <unistd.h>

struct sigaction old_act;
int main( void )
{
    extern void sig_handler();

    struct sigaction act;
    sigset_t set;
    sigemptyset( &set );
    sigaddset( &set, SIGINT );

    act.sa_flags = 0;
    act.sa_mask = set;
    act.sa_handler = &sig_handler;
    sigaction( SIGINT, &act, &old_act );

    return EXIT_SUCCESS;
}
void sig_handler( )
{
    dettach ISR;
    //invoke the default signal handler
    old_act.sa_handler(); 
}
