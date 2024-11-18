/*
 * /dev/bLED using the resource manager library
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
#define NUM_ENTS 2
struct my_attr {
	iofunc_attr_t base; /* must always be first */
	char *content; //the file content
};

extern void init_serat91sam9x(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
extern void board_init(void);



static IOFUNC_ATTR_T led_attrs [NUM_ENTS];
static const char *fname[NUM_ENTS] = { "D1", "D2" };


int my_write (resmgr_context_t *ctp, io_write_t *msg, iofunc_ocb_t *ocb)
{
	int status;
	static char *buf;

	// verify that the device is opened for write
	if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK)
		return (status);
	// check for and handle an XTYPE override
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
		return(ENOSYS);

	/* set up the number of bytes (from the client's write()) */
	_IO_SET_WRITE_NBYTES (ctp, msg->i.nbytes);
	buf = (char *) malloc(msg->i.nbytes + 1);
	if (buf == NULL)
		return(ENOMEM);

	// Read data from the message buffer.
	if (resmgr_msgread(ctp,buf,msg->i.nbytes,sizeof(msg->i))== -1){
		free (buf);
		return (errno);
	}
	buf [msg->i.nbytes] = '\0'; 

	// Gain access to LED registers
	uintptr_t OERPtr, CODRPtr, SODRPtr;
	OERPtr = mmap_device_io(4, PIOD_BASE+PIO_OER);
	CODRPtr = mmap_device_io(4, PIOD_BASE+PIO_CODR);
	SODRPtr = mmap_device_io(4, PIOD_BASE+PIO_SODR);

	// enable LED output
	out32(OERPtr, 0x80000001);
	// clear registers PIO_CODR and PIO_SODR
	out32(CODRPtr, 0x0);
	out32(SODRPtr, 0x0);

	int offset = ocb -> attr -> base.inode - 1;
	switch(offset){
	case 0: //LED D1
		if(buf[0] == '0') //turn off LED
			out32(SODRPtr, 0x1);
		else if(buf[0] == '1')
			out32(CODRPtr, 0x1);
		break;
	case 1:  //LED D2
		if(buf[0] == '0') //turn off LED
			out32(SODRPtr, 0x80000000);
		else if(buf[0] == '1')
			out32(CODRPtr, 0x80000000);
		break;
	}
	// update time attributes
	if (msg->i.nbytes > 0) {
		ocb->attr->base.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
		iofunc_time_update(ocb->attr);
	}
	return (_RESMGR_NPARTS (0));
}
static int
my_open (resmgr_context_t *ctp, io_open_t *msg, IOFUNC_ATTR_T *attr, void *extra)
{
	static char *path_buf;


	if (msg -> connect.path [0] == 0) {
		return (iofunc_open_default (ctp, msg, &attr -> base, extra));
	} else{
    	path_buf = (char *) malloc(msg -> connect.path_len + 1);
    	if (path_buf == NULL) return(ENOMEM);

    	strcpy(path_buf, msg -> connect.path);
    	/* just in case the text is not NULL terminated */
    	path_buf [msg -> connect.path_len] = '\0';

    	int offset = 0;
    	if (strcmp(path_buf, fname[0])==0) offset = 0;
    	else if (strcmp(path_buf, fname[1])==0) offset = 1;

        return (iofunc_open_default (ctp, msg, &led_attrs[offset].base, extra));
	}
}

int
dirent_size (char *fname)
{
	return (ALIGN (sizeof (struct dirent) - 4 + strlen (fname)));
}
struct dirent *
dirent_fill (struct dirent *dp, int inode, int offset, char *fname)
{
	dp -> d_ino = inode;
	dp -> d_offset = offset;
	strcpy (dp -> d_name, fname);
	dp -> d_namelen = strlen (dp -> d_name);
	dp -> d_reclen = ALIGN (sizeof (struct dirent) - 4 + dp -> d_namelen);
	return ((struct dirent *) ((char *) dp + dp -> d_reclen));
}
static int
my_read_dir (resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb)
{

	int nbytes;
	int nleft;
	struct dirent *dp;
	char *reply_msg;

	// allocate a buffer for the reply
	reply_msg = calloc (1, msg -> i.nbytes);
	if (reply_msg == NULL) {
		return (ENOMEM);
	}
	// assign output buffer
	dp = (struct dirent *) reply_msg;
	// we have "nleft" bytes left
	nleft = msg -> i.nbytes;
	while (ocb -> offset < NUM_ENTS) {


		// see how big the result is
		nbytes = dirent_size (fname[ocb -> offset]);
		// do we have room for it?
		if (nleft - nbytes >= 0) {
			// fill the dirent, and advance the dirent pointer
			dp = dirent_fill (dp, ocb -> offset + 1, ocb -> offset, fname[ocb -> offset]);
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
	MsgReply (ctp -> rcvid, (char *) dp - reply_msg, reply_msg, (char *) dp - reply_msg);
	// release our buffer
	free (reply_msg);
	// tell resource manager library we already did the reply
	return (_RESMGR_NOREPLY);
}

static int
my_read (resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb)
{
	int sts;
	// use the helper function to decide if valid
	if ((sts = iofunc_read_verify (ctp, msg, ocb, NULL)) != EOK) {
		return (sts);
	}
	// decide if we should perform the "file" or "dir" read
	if (S_ISDIR (ocb -> attr ->base.mode)) {
		return (my_read_dir (ctp, msg, ocb));
	} else {
		return (EBADF);
	}
}
int
main (int argc, char **argv)
{
	dispatch_t *dpp;
	resmgr_attr_t resmgr_attr;
	resmgr_context_t *ctp;
	resmgr_connect_funcs_t connect_func;
	resmgr_io_funcs_t io_func;
	IOFUNC_ATTR_T attr;
	int i;


	// create the dispatch structure
	if ((dpp = dispatch_create ()) == NULL) {
		perror ("Unable to dispatch_create\n");
		exit (EXIT_FAILURE);
	}
	// initialize the various data structures
	memset (&resmgr_attr, 0, sizeof (resmgr_attr));
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;
	// bind default functions into the outcall tables
	iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_func, _RESMGR_IO_NFUNCS, &io_func);
	// create and initialize the attributes structure for the directory
	iofunc_attr_init (&attr.base, S_IFDIR | 0555, 0, 0);
	attr.base.inode = NUM_ENTS + 1;

	attr.base.nbytes = NUM_ENTS;

	for (i = 0; i < NUM_ENTS; i++) {
		iofunc_attr_init (&led_attrs [i].base, S_IFREG | 0444, 0, 0);
		led_attrs [i].base.inode = i + 1;
		led_attrs [i].base.nbytes = 0;
	}

	// add our functions;
	connect_func.open = my_open;
	io_func.read = my_read;
	io_func.write = my_write;

	// establish a name in the pathname space
	if (resmgr_attach (dpp, &resmgr_attr, "/dev/bLED", _FTYPE_ANY, _RESMGR_FLAG_DIR, &connect_func, &io_func, &attr) == -1) {
		perror ("Unable to resmgr_attach\n");
		exit (EXIT_FAILURE);
	}
	// allocate a context
	ctp = resmgr_context_alloc (dpp);
	// wait here forever, handling messages
	while (1) {
		if ((ctp = resmgr_block (ctp)) == NULL) {
			perror ("Unable to resmgr_block\n");
			exit (EXIT_FAILURE);
		}
		resmgr_handler (ctp);
	}
}
