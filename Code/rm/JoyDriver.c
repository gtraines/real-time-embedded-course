/*
 * /dev/Joysticks using the resource manager library
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <sys/mman.h>
struct my_attr;
#define IOFUNC_ATTR_T struct my_attr
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <hw/inout.h>
#include <string.h>
#include "at91sam9m10.h"
#define ALIGN(x) (((x) + 3) & ~3)
#define NUM_ENTS 7
struct my_attr {
	iofunc_attr_t base; /* must always be first */
	char *content; //the file content
};

extern void init_serat91sam9x(unsigned, unsigned, unsigned, unsigned, unsigned,
		unsigned);
extern void board_init(void);

static IOFUNC_ATTR_T joy_attrs [NUM_ENTS];
static const char *fname[NUM_ENTS] = { "Button_L", "Button_R", "Joy_Down",
		"Joy_Left", "Joy_Push", "Joy_Right", "Joy_Up" };

static int my_open(resmgr_context_t *ctp, io_open_t *msg, IOFUNC_ATTR_T *attr,
		void *extra) {

	if (msg -> connect.path[0] == 0) { // the directory (/dev/Joysticks)
		return (iofunc_open_default(ctp, msg, &attr -> base, extra));
	} else {

		int offset = 0;
		int counter = 0;
		for (; counter < NUM_ENTS; counter++) {
			if (strcmp(msg -> connect.path, fname[counter]) == 0) {
				offset = counter;
				break;
			}
		}
		return (iofunc_open_default(ctp, msg, &joy_attrs[offset].base, extra));
	}
}
int dirent_size(char *fname) {
	return (ALIGN (sizeof (struct dirent) - 4 + strlen (fname)));
}
struct dirent *
dirent_fill(struct dirent *dp, int inode, int offset, char *fname) {
	dp -> d_ino = inode;
	dp -> d_offset = offset;
	strcpy(dp -> d_name, fname);
	dp -> d_namelen = strlen(dp -> d_name);
	dp -> d_reclen = ALIGN (sizeof (struct dirent) - 4 + dp -> d_namelen);
	return ((struct dirent *) ((char *) dp + dp -> d_reclen));
}
static int my_read_dir(resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb) {
	int nbytes;
	int nleft;
	struct dirent *dp;
	char *reply_msg;

	// allocate a buffer for the reply
	reply_msg = calloc(1, msg -> i.nbytes);
	if (reply_msg == NULL) {
		return (ENOMEM);
	}
	// assign output buffer
	dp = (struct dirent *) reply_msg;
	// we have "nleft" bytes left
	nleft = msg -> i.nbytes;
	while (ocb -> offset < NUM_ENTS) {

		// see how big the result is
		nbytes = dirent_size(fname[ocb -> offset]);
		// do we have room for it?
		if (nleft - nbytes >= 0) {
			// fill the dirent, and advance the dirent pointer
			dp = dirent_fill(dp, ocb -> offset + 1, ocb -> offset,
					fname[ocb -> offset]);
			// move the OCB offset

			ocb -> offset++;
			// account for the bytes we just used up
			nleft -= nbytes;
		} else {
			// don't have any more room, stop
			break;
		}
	}
	// return info back to the client
	MsgReply(ctp -> rcvid, (char *) dp - reply_msg, reply_msg, (char *) dp
			- reply_msg);
	// release our buffer
	free(reply_msg);
	// tell resource manager library we already did the reply
	return (_RESMGR_NOREPLY);
}
static int my_read_file(resmgr_context_t *ctp, io_read_t *msg,
		iofunc_ocb_t *ocb) {

	static char flag = 0;

	// not expecting messages of extended types
	if ((msg -> i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) 
		return (ENOSYS);
	if (!flag) {
		uint32_t input;
		uintptr_t buttonEnablePtr, buttonInputPtr;

    	// Gain access to Pushbutton and Joystick registers
		buttonEnablePtr = mmap_device_io(4, PIOB_BASE + PIO_PER);
		buttonInputPtr = mmap_device_io(4, PIOB_BASE + PIO_PDSR);

		//enable PIOB input pins for joysticks
		out32(buttonEnablePtr, 0x0007C0C0);

		//read inputs
		input = in32(buttonInputPtr);
		input ^= 0xFFFFFFFF;

		char* string = "0";
		int fileoffset = ocb -> attr -> base.inode - 1;
		switch (fileoffset) {
		case 0: //Push Button Left
			if (input & 0x40) string = "1";
			break;
		case 1: //Push Button Right
			if (input & 0x80) string = "1";
			break;
		case 2: //Joystick Down
			if (input & 0x20000) string = "1";
			break;
		case 3: //Joystick Left
			if (input & 0x4000) string = "1";
			break;
		case 4: //Joystick Push
			if (input & 0x40000) string = "1";
			break;
		case 5: //Joystick Right
			if (input & 0x8000) string = "1";
			break;
		case 6: //Joystick Up
			if (input & 0x10000) string = "1";
			break;
		}
		MsgReply(ctp->rcvid, strlen(string), string, strlen(string));
		flag = 1;
	} else {
		MsgReply(ctp -> rcvid, EOK, NULL, 0);
		flag = 0;
	}
	// already done the reply ourselves
	return (_RESMGR_NOREPLY);
}
static int my_read(resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb) {
	int sts;
	// use the helper function to decide if valid
	if ((sts = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK) {
		return (sts);
	}
	// decide if we should perform the "file" or "dir" read
	if (S_ISDIR (ocb -> attr ->base.mode)) {
		return (my_read_dir(ctp, msg, ocb));
	} else if (S_ISREG (ocb -> attr ->base.mode)) {
		return (my_read_file(ctp, msg, ocb));
	} else {
		return (EBADF);
	}
}
int main(int argc, char **argv) {
	dispatch_t *dpp;
	resmgr_attr_t resmgr_attr;
	resmgr_context_t *ctp;
	resmgr_connect_funcs_t connect_func;
	resmgr_io_funcs_t io_func;
	IOFUNC_ATTR_T attr;
	int i;

	//printf("Resource Manager\n");

	// create the dispatch structure
	if ((dpp = dispatch_create()) == NULL) {
		perror("Unable to dispatch_create\n");
		exit(EXIT_FAILURE);
	}
	// initialize the various data structures
	memset(&resmgr_attr, 0, sizeof(resmgr_attr));
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;
	// bind default functions into the outcall tables
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_func, _RESMGR_IO_NFUNCS,
			&io_func);
	// create and initialize the attributes structure for the directory
	iofunc_attr_init(&attr.base, S_IFDIR | 0555, 0, 0);
	attr.base.inode = NUM_ENTS + 1; //

	attr.base.nbytes = NUM_ENTS; //

	for (i = 0; i < NUM_ENTS; i++) {
		iofunc_attr_init(&joy_attrs[i].base, S_IFREG | 0444, 0, 0);
		joy_attrs[i].base.inode = i + 1;
		joy_attrs[i].base.nbytes = 0;
	}

	// add our functions; we're only interested in io_open and io_read
	connect_func.open = my_open;
	io_func.read = my_read;
	// establish a name in the pathname space
	if (resmgr_attach(dpp, &resmgr_attr, "/dev/Joysticks", _FTYPE_ANY,
			_RESMGR_FLAG_DIR, &connect_func, &io_func, &attr) == -1) {
		perror("Unable to resmgr_attach\n");
		exit(EXIT_FAILURE);
	}
	// allocate a context
	ctp = resmgr_context_alloc(dpp);
	// wait here forever, handling messages
	while (1) {
		if ((ctp = resmgr_block(ctp)) == NULL) {
			perror("Unable to resmgr_block\n");
			exit(EXIT_FAILURE);
		}
		resmgr_handler(ctp);
	}
}
