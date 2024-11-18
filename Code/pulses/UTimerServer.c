/*
*  UTimer Server: receives pulses from a periodic QNX timer;
*  and manages Utimer objects installed by clients.
*  It sends a pulse to a client when its Utimer expires.
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include "UTimer.h"

void setupPulseAndTimer (void);
void handleAPulse (void);
void handleAMessage (int rcvid, ClientMsgT *cmsg);
void installUTimer(UTimer_Node* ptr);
void cancelUTimer(int UtimerID);

int chid;
int unique_timerID = 50;
name_attach_t				*attach;
struct sigevent			pulseEventFromTimer;
struct UTimer_List	*tlist;

int main (void)  {
	int rcvid;  // process ID of the message sender
	MessageT externalMsg;

	// set up the pulse and timer
	setupPulse ();
	setupOSTimer ();

	tlist=(struct UTimer_List*)malloc(sizeof(struct UTimer_List));
	for (;;) {
		rcvid = MsgReceive (attach->chid, &externalMsg,
							sizeof (externalMsg), NULL);
		if (rcvid == 0) {
			handleAPulse ();
		} else {
			handleAMessage (rcvid, &externalMsg.cmsg);
		}
	}
	return (EXIT_SUCCESS);
}

void setupPulse (void) {
	int  coid;       // connection ID

	if ( (attach = name_attach( NULL, MY_SERV, 0 )) == NULL){
		fprintf (stderr, "Couldn't create channel!\n");
		perror (NULL);
		exit (EXIT_FAILURE);
	}
	// create a connection back to ourselves for receiving pulses
	coid = ConnectAttach (0, 0, attach->chid, 0, 0);
	if (coid == -1) {
		fprintf (stderr, "Couldn't ConnectAttach to self!\n");
		perror (NULL);
		exit (EXIT_FAILURE);
	}
	// set up a pulse event with code MT_TIMER
	SIGEV_PULSE_INIT (&pulseEventFromTimer, coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER, 0);
}

void setupOSTimer (void) {
	timer_t             timerid;    // timer ID for the OS timer
	struct itimerspec   timer;      // the timer data structure

	// create an OS timer, binding it to the pulse event
	if (timer_create (CLOCK_REALTIME, &pulseEventFromTimer,
						&timerid) == -1) {
		fprintf (stderr, "Timer fails, errno %d\n", errno);
		perror (NULL);
		exit (EXIT_FAILURE);
	}

	// setup the timer to deliver a pulse once per second
	timer.it_value.tv_sec = 1;
	timer.it_value.tv_nsec = 0;
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_nsec = 0;

	// start the OS timer
	timer_settime (timerid, 0, &timer, NULL);
}

void removeUTimer(UTimer_Node *ptr){
	UTimer_Node *p;

	p = tlist->head;
	while(p != ptr)
		p = p->next;

	if (p == tlist->head)
	{   //remove the old head
		tlist->head = p->next;
		if (p->next != NULL)
			p->next->pre = NULL;
	} else {
		p->pre->next = p->next;
		if (p->next != NULL)
			p->next->pre = p->pre;
	}
	p->next = NULL;
	p->pre = NULL;
}

int isTimeOut(UTimer_Node *ptr){
	if (ptr->utimer.secondsLeft == 0)
		return 1;
	else return 0;
}

void handleAPulse (void) {
	UTimer_Node *ptr = NULL;
	UTimer_Node *psaved = NULL;

	ptr = tlist->head;
	if (ptr != NULL) psaved = ptr->next;
	while(ptr != NULL) {
		ptr->utimer.secondsLeft--;
		if(isTimeOut(ptr)) {
			removeUTimer(ptr);
			//deliver pulse if necessary
			ptr->pulseEvent.sigev_value.sival_int 
					= ptr->utimer.UtimerID;
			MsgDeliverEvent(ptr->rcvid, &ptr->pulseEvent);
			free(ptr);
			printf ("The UTimer object %d has expired, pulse the client!\n", ptr->utimer.UtimerID);
		}
		ptr = psaved;
		if (ptr !=NULL) psaved = ptr->next;
	}
}

void handleAMessage (int rcvid, ClientMsgT *cmsg) {
	UTimer_Node *np;
	ServerMessageT *replyMsg;

	switch (cmsg -> messageType) {

case MT_REGISTER:
	//create a UTimer Node
	unique_timerID++;
	np = (UTimer_Node*)malloc(sizeof(UTimer_Node));
	np->utimer.UtimerID = unique_timerID;
	np->utimer.priority = cmsg->priority;
	np->utimer.secondsLeft = cmsg->seconds;
	np->pulseEvent = cmsg->pulseEvent;
	np->rcvid = rcvid;
	np->pre = NULL;
	np->next = NULL;

	//insert the timer
	installUTimer(np);
	printf ("The UTimer object %d has been installed for %d seconds!\n", np->utimer.UtimerID, np->utimer.secondsLeft);

	replyMsg = (ServerMessageT *)malloc(sizeof(ServerMessageT));
	replyMsg->messageType = MT_REGISTERED;
	replyMsg->UtimerID = np->utimer.UtimerID;
	MsgReply(rcvid, EOK, replyMsg, sizeof(*replyMsg));
	free(replyMsg);
	break;

case MT_CANCEL:
	//search for the timer and remove it
	cancelUTimer(cmsg->UtimerID);

	replyMsg = (ServerMessageT *)malloc(sizeof(ServerMessageT));
	replyMsg->messageType = MT_CANCELED;
	MsgReply(rcvid, EOK, replyMsg, sizeof(*replyMsg));
	free(replyMsg);
	break;
	}
}

void installUTimer(UTimer_Node* ptr) {
	UTimer_Node *np;

	np = tlist->head;
	if(np == NULL)
	{   //no UTimer Node exists
		tlist->head = ptr;
	}
	else if(ptr->utimer.priority > np->utimer.priority)
	{   //the head node has a lower priority
		ptr->pre = NULL;
		ptr->next = np;
		np->pre = ptr;
		tlist->head = ptr;
	}
	else
	{   //find a spot for it in the list
		UTimer_Node *prev;
		while(np != NULL) {
			if(ptr->utimer.priority > np->utimer.priority) {
				np->pre->next = ptr;
				ptr->next = np;
				ptr->pre = np->pre;
				np->pre = ptr;
				break;
			}
			else {
				prev = np;
				np = np->next;
			}
		}

		if(np == NULL) {
			prev->next = ptr;
			ptr->pre = prev;
			ptr->next = NULL;
		}
	}
}

void cancelUTimer(int UtimerID){
	UTimer_Node *p;

	p = tlist->head;
	while((p!= NULL) && (p->utimer.UtimerID != UtimerID))
		p = p->next;

	if (p != NULL) {
		if (p == tlist->head) {
			tlist->head = p->next;

			if (p->next != NULL)
				p->next->pre = NULL;
		} else {
			p->pre->next = p->next;
			if (p->next != NULL)
				p->next->pre = p->pre;
		}
		p->next = NULL;
		p->pre = NULL;
		free(p);
	}
}
