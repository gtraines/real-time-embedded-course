#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define INT_SIGNAL_A 0x0001
#define INT_SIGNAL_B 0x0002
#define INT_SIGNAL_C 0x0004
#define INT_SIGNAL_D 0x0008
#define INT_SIGNAL_E 0x0010

static WORD int_Signals;

void ISR_A() {
	//move data from device to main memory if necessary
	//reset Device A
	int_Signals |= INT_SIGNAL_A;
}
void handler_A() {
	//fully process service request from A
}
void ISR_B() {
	//move data from device to main memory if necessary
	//reset Device B
	int_Signals |= INT_SIGNAL_B;
}
void handler_B() {
	//fully process service request from B
}
void ISR_C() {
	//move data from device to main memory if necessary
	//reset Device C
	int_Signals |= INT_SIGNAL_C;
}
void handler_C() {
	//fully process service request from C
}
void ISR_D() {
	//move data from device to main memory if necessary
	//reset Device D
	int_Signals |= INT_SIGNAL_D;
}
void handler_D() {
	//fully process service request from D
}
void ISR_E() {
	//move data from device to main memory if necessary
	//reset Device E
	int_Signals |= INT_SIGNAL_E;
}
void handler_E() {
	//fully process service request from E
}

int main() {
	WORD highestPriorityFlag;
	while (1) {
		//wait until the arrival of an interrupt signal
		while (int_Signals == 0);
		highestPriorityFlag = INT_SIGNAL_A;

		interrupt_disable();
		while ((int_Signals & highestPriorityFlag) == 0)
			//check the next interrupt signal
			highestPriorityFlag <<= 1;
		//reset this signal, which is to be serviced
		int_Signals &= ~highestPriorityFlag;
		interrupt_disable();

		switch (highestPriorityFlag) {
		case INT_SIGNAL_A:
			handler_A();
			break;
		case INT_SIGNAL_B:
			handler_B();
			break;
		case INT_SIGNAL_C:
			handler_C();
			break;
		case INT_SIGNAL_D:
			handler_D();
			break;
		case INT_SIGNAL_E:
			handler_E();
			break;
		}
	}
}
