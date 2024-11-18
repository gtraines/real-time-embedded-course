#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void msleep(unsigned long msec) {
	struct timespec interval;
	struct timespec remainder;

	interval.tv_sec = msec / 1000;
	interval.tv_nsec = (msec % 1000) * (1000 * 1000);

	if (nanosleep(&interval, &remainder) == -1) {
		perror("nanosleep");
	}
}

int Button_Left_Pressed() {
	int fd = open("/dev/Joysticks/Button_L", O_RDONLY);
	char *text = malloc(sizeof(char));
	read(fd, text, 1);
	close(fd);
	if (text[0] == '1')	return 1;
	return 0;
}
int Button_Right_Pressed() {
	int fd = open("/dev/Joysticks/Button_R", O_RDONLY);
	char *text = malloc(sizeof(char));
	read(fd, text, 1);
	close(fd);
	if (text[0] == '1')	return 1;
	return 0;
}
int Joy_Right_Pressed() {
	int fd = open("/dev/Joysticks/Joy_Right", O_RDONLY);
	char *text = malloc(sizeof(char));
	read(fd, text, 1);
	close(fd);
	if (text[0] == '1')	return 1;
	return 0;
}
int Joy_Left_Pressed() {
	int fd = open("/dev/Joysticks/Joy_Left", O_RDONLY);
	char *text = malloc(sizeof(char));
	read(fd, text, 1);
	close(fd);
	if (text[0] == '1')	return 1;
	return 0;
}
int Joy_Up_Pressed() {
	int fd = open("/dev/Joysticks/Joy_Up", O_RDONLY);
	char *text = malloc(sizeof(char));
	read(fd, text, 1);
	close(fd);
	if (text[0] == '1')	return 1;
	return 0;
}
int Joy_Down_Pressed() {
	int fd = open("/dev/Joysticks/Joy_Down", O_RDONLY);
	char *text = malloc(sizeof(char));
	read(fd, text, 1);
	close(fd);
	if (text[0] == '1')	return 1;
	return 0;
}
int Joy_Push_Pressed() {
	int fd = open("/dev/Joysticks/Joy_Push", O_RDONLY);
	char *text = malloc(sizeof(char));
	read(fd, text, 1);
	close(fd);
	if (text[0] == '1')	return 1;
	return 0;
}
void turnoff_LED_D1() {
	int fd = open("/dev/bLED/D1", O_WRONLY);
	char *text = malloc(sizeof(char));
	text[0] = '0';
	write(fd, text, 1);
	close(fd);
}
void turnon_LED_D1() {
	int fd = open("/dev/bLED/D1", O_WRONLY);
	char *text = malloc(sizeof(char));
	text[0] = '1';
	write(fd, text, 1);
	close(fd);
}
void turnoff_LED_D2() {
	int fd = open("/dev/bLED/D2", O_WRONLY);
	char *text = malloc(sizeof(char));
	text[0] = '0';
	write(fd, text, 1);
	close(fd);
}
void turnon_LED_D2() {
	int fd = open("/dev/bLED/D2", O_WRONLY);
	char *text = malloc(sizeof(char));
	text[0] = '1';
	write(fd, text, 1);
	close(fd);
}

void setoff_LED() {
	turnoff_LED_D1();
	turnoff_LED_D2();
}
void seton_LED() {
	turnon_LED_D1();
	turnon_LED_D2();
}

int main(int argc, char *argv[]) {
	while (1) {
		msleep(500);
		if (Button_Left_Pressed()>0) {
			seton_LED();
			continue;
		}
		if (Button_Right_Pressed()>0) {
			setoff_LED();
			continue;
		}
		if (Joy_Left_Pressed()>0) {
			turnon_LED_D1();
			continue;
		}
		if (Joy_Up_Pressed()>0) {
			turnon_LED_D2();
			continue;
		}
		if (Joy_Right_Pressed()>0) {
			turnoff_LED_D2();
			continue;
		}
		if (Joy_Down_Pressed()>0) {
			turnoff_LED_D1();
			continue;
		}
		if (Joy_Push_Pressed()>0) {
			setoff_LED();
		}
	}
	return EXIT_SUCCESS;
}
