#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>
#include <time.h>
#include <signal.h>
#include <sys/siginfo.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/syspage.h>
#include "UTimer.h"

int srv_coid, rcvid, chid;
struct sigevent	pulseEventFromServer;
struct _pulse		pulseObject;
ClientMsgT			cmsg;
ServerMessageT	smsg;

void jobBeforeCheckingPulse(int timerID){
	// here is a chance to cancel the installed timer.
	printf("Client processing some job before pulse\n\n");
}
void jobUponPulse(void){
	printf("Client processing some job upon pulse\n\n");
}

void registerATimer(int timeoutSec){
	int myTimerID;

	cmsg.messageType = MT_REGISTER;
	cmsg.priority = 10;
	//set the number of seconds to wait for
	cmsg.seconds = timeoutSec;

	// The pulse event to be registered to the Utimer Server
	cmsg.pulseEvent = pulseEventFromServer;

	MsgSend( srv_coid, &cmsg, sizeof(cmsg), &smsg, sizeof(smsg) );
	if (smsg.messageType == MT_REGISTERED){
		myTimerID = smsg.UtimerID; //used for timer canceling
		printf("Client %d has registered a UTimer %d!\n", getpid(), myTimerID);
	}
	jobBeforeCheckingPulse(myTimerID);

	rcvid = MsgReceivePulse(chid, &pulseObject,
							sizeof(pulseObject), NULL);
	printf("Client %d has received a pulse with code %d\n", 
					getpid(), pulseObject.code);
	printf("My UTimer with the ID %d has expired\n", myTimerID);
	if (pulseObject.code == CODE_SERVER){
		//timeout, now do some jobs
		jobUponPulse();
	}
}

void setupPulseEvent(void){
	int coid;

	/* A communication channel*/
	chid = ChannelCreate( 0 );
	/* A connection to the channel on which to receive pulses*/
	coid = ConnectAttach( 0, 0, chid, _NTO_SIDE_CHANNEL, 0 );

	// set up a pulse event to be registered to the server
	SIGEV_PULSE_INIT( &pulseEventFromServer, coid, SIGEV_PULSE_PRIO_INHERIT, CODE_SERVER, 0 );
}

int main( int argc, char **argv)
{
	/* find the Utimer Server */
	if ( (srv_coid = name_open( MY_SERV, 0 )) == -1) {
		printf("failed to find server, errno %d\n", errno );
		exit(1);
	}
	setupPulseEvent();
	registerATimer(10);
	return 0;
}
