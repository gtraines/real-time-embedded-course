#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/neutrino.h>

int main(int argc, char *argv[]) {

	intrspin_t* spinlock;
	spinlock = (intrspin_t*) malloc(sizeof(intrspin_t));
	memset(spinlock, 0, sizeof(*spinlock));

	ThreadCtl(_NTO_TCTL_IO, NULL);

	InterruptLock( spinlock );

	printf("Start of critcal data modification\n");
	sleep(3);
	printf("End of critcal data modification\n");

	InterruptUnlock(spinlock);
	return EXIT_SUCCESS;
}

