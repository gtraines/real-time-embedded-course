/*
*  math_of_OZ.c
*  mountpoint: /dev/mathoz
*/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#define ALIGN(x)	(((x) + 3) & ~3)
#define NUM_ENTS    4
#define CAPACITY	10

struct my_attr;
#define IOFUNC_ATTR_T struct my_attr
#include <sys/iofunc.h>
#include <sys/dispatch.h>
struct my_attr {
	iofunc_attr_t base; /* must always be first */
	char *content; /* the file content */
};

static IOFUNC_ATTR_T math_attrs [NUM_ENTS];
const char *fname[4] = { "o1", "operator", "o2", "result" };
const char defent[4] = { '1', '+', '1', '2' };

static int my_open(resmgr_context_t *ctp, io_open_t *msg, iofunc_attr_t *attr,void *extra) {
   static char *path_buf;

   if (msg -> connect.path[0] == 0) { // open the directory
	   return (iofunc_open_default(ctp, msg, attr, extra));
   } else {
	   printf("my_open: resource name is %s.\n", msg -> connect.path);
	   path_buf = (char *) malloc(msg -> connect.path_len + 1);
	   if (path_buf == NULL) return (ENOMEM);
	   strcpy(path_buf, msg -> connect.path);
	   path_buf[msg -> connect.path_len] = '\0';
	   int dir_offset = 0;
	   for (dir_offset = 0; dir_offset < NUM_ENTS; dir_offset++) {
		   if (strcmp(path_buf, fname[dir_offset]) == 0)
			   break;
	   }
	   return (iofunc_open_default(ctp, msg, &math_attrs[dir_offset].base, extra));
   }
}

int my_write(resmgr_context_t *ctp, io_write_t *msg, iofunc_ocb_t *ocb) {
	int status;
	static char *buf;
	int dir_offset;

	// verify that the resource is writable
	if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK)
		return (status);
	// not expecting messages of extended types
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
		return (ENOSYS);

	/* set up the number of bytes (from client's write()) */
	_IO_SET_WRITE_NBYTES (ctp, msg->i.nbytes);
	buf = (char *) malloc(msg->i.nbytes + 1);
	if (buf == NULL) return (ENOMEM);

	// Read a message from a client. Use sizeof(msg->i) as offset 
	// because data immediately follows the _io_write structure.
	if (resmgr_msgread(ctp, buf, msg->i.nbytes, sizeof(msg->i)) == -1) {
		free(buf);
		return (errno);
	}

	// Process the message
	buf[msg->i.nbytes] = '\0';
	printf("my_write: input is %s\n", buf);

	dir_offset = ocb -> attr -> base.inode - 1;
	if (dir_offset < 3) { //cannot write to "result"

		if (math_attrs[dir_offset].content != NULL)
			memset(math_attrs[dir_offset].content, 0, CAPACITY);
		strcpy(math_attrs[dir_offset].content, buf);
		math_attrs[dir_offset].base.nbytes = strlen(buf);

		// if any data written, update POSIX structures
		if (msg->i.nbytes > 0) {
			ocb->attr->base.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
			iofunc_time_update(ocb->attr);
		}
	}
	// free the buffer
	free(buf);
	return (_RESMGR_NPARTS (0));
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
	if (reply_msg == NULL)
		return (ENOMEM);

	// assign output buffer
	dp = (struct dirent *) reply_msg;

	nleft = msg -> i.nbytes;
	while (ocb -> offset < NUM_ENTS) {
		// the size of the current entry
		nbytes = dirent_size(fname[ocb -> offset]);

		if (nleft - nbytes >= 0) {
			// fill the dirent, and advance the dirent pointer
			dp = dirent_fill(dp, ocb -> offset + 1, ocb -> offset,
				fname[ocb -> offset]);
			// move the OCB offset
			ocb -> offset++;
			// account for the bytes we just used up
			nleft -= nbytes;
		} else // don't have any more room, stop
			break;
	}

	// return info back to the client
	MsgReply(ctp -> rcvid,  (char *) dp - reply_msg,   reply_msg,  (char *) dp - reply_msg);

	// release our buffer
	free(reply_msg);
	// tell resource manager library we already did the reply
	return (_RESMGR_NOREPLY);
}
static void calculate_result() {
	int op1 = atoi(math_attrs[0].content);
	int op2 = atoi(math_attrs[2].content);
	int result = 0;
	if (strncmp(math_attrs[1].content, "+", 1) == 0)
		result = op1 + op2;
	else if (strncmp(math_attrs[1].content, "-", 1) == 0)
		result = op1 - op2;
	else if (strncmp(math_attrs[1].content, "X", 1) == 0)
		result = op1 * op2;
	else if (strncmp(math_attrs[1].content, "%", 1) == 0) {
		if (op2 == 0)
			op2 = 1;
		result = op1 % op2;
	}
	printf("Result: %d %c %d = %d.\n\n", op1, math_attrs[1].content[0],op2, result);
	if (math_attrs[3].content != NULL)
		memset(math_attrs[3].content, 0, CAPACITY);
	itoa(result, math_attrs[3].content, CAPACITY);
	math_attrs[3].base.nbytes = strlen(math_attrs[3].content);
}
static int my_read_file(resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb) {
	// not expecting messages of extended types
	if ((msg -> i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		return (ENOSYS);
	}

	int dir_offset = ocb -> attr -> base.inode - 1;

	if ((dir_offset == 3) & (ocb -> offset == 0)) 
	{  //this is the first time /dev/mathoz/result is read
		calculate_result();
	}

	// check the resource to see how many bytes are left
	int nleft = math_attrs[dir_offset].base.nbytes - ocb -> offset;

	// return the minimum of nleft and the number requested
	int nbytes = min (nleft, msg -> i.nbytes);
	if (nbytes > 0) {
		MsgReply(ctp -> rcvid, nbytes, math_attrs[dir_offset].content+ ocb-> offset, nbytes);
		ocb -> offset += nbytes; 
		// update flags & time attributes
		ocb -> attr -> base.flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;
		iofunc_time_update(ocb->attr);
	}
	else {
		// nothing to return, indicate End Of File
		MsgReply (ctp -> rcvid, EOK, NULL, 0);
	}
	// already done the reply ourselves
	return (_RESMGR_NOREPLY);
}

static int my_read(resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb) {
	// verify the resource is readable
	int sts;
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
	IOFUNC_ATTR_T dir_attr;

	// Setup the attributes for the directory
	iofunc_attr_init(&dir_attr.base, S_IFDIR | 0555, 0, 0);
	dir_attr.base.inode = NUM_ENTS + 1; //
	dir_attr.base.nbytes = NUM_ENTS; //
	// Setup the attributes for each resource under /dev/mathoz
	int i;
	for (i = 0; i < 4; i++) {
		iofunc_attr_init(&math_attrs[i].base, S_IFREG | 0444, 0, 0);
		math_attrs[i].base.inode = i + 1;
		math_attrs[i].base.nbytes = 1;
		math_attrs[i].content = (char*) malloc(CAPACITY);
		math_attrs[i].content[0] = defent[i];
	}

	// bind default message handler functions
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_func, _RESMGR_IO_NFUNCS, &io_func);
	// override with our own message handlers;
	connect_func.open = my_open;
	io_func.read = my_read;
	io_func.write = my_write;

	// initialize attributes for the resource manager
	memset(&resmgr_attr, 0, sizeof(resmgr_attr));
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;

	// create the dispatch structure
	if ((dpp = dispatch_create()) == NULL) {
		perror("Unable to dispatch_create\n");
		exit(EXIT_FAILURE);
	}
	// register the filesystem resource manager at /dev/mathoz in the pathname space
	if (resmgr_attach(dpp, &resmgr_attr, "/dev/mathoz", _FTYPE_ANY,_RESMGR_FLAG_DIR,&connect_func,&io_func,&dir_attr)==-1){
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
