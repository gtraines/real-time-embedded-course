/*
 *  atoz.c
 *
 *  /dev/atoz using the resource manager library
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

struct my_attr;
#define IOFUNC_ATTR_T struct my_attr 

#include <sys/iofunc.h>
#include <sys/dispatch.h>

#define ALIGN(x) (((x) + 3) & ~3)
#define NUM_ENTS            26

struct my_attr { 
	iofunc_attr_t base; /* must always be first */
	char *content; //the file content
};


static  IOFUNC_ATTR_T   atoz_attrs [NUM_ENTS];


int
my_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
	int status;
	static char *buf;
	int offset;
	char fname;

    // verify that the device is opened for write
	if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK)
		return (status);
    // check for and handle an XTYPE override
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
		return(ENOSYS);

	/* set up the number of bytes (returned by client’s write()) */
	_IO_SET_WRITE_NBYTES (ctp, msg->i.nbytes);
	buf = (char *) malloc(msg->i.nbytes + 1);
	if (buf == NULL)
		return(ENOMEM);
	/*
	* Reread the data from the sender’s message buffer.
	* We’re not assuming that all of the data fit into the
	* resource manager library’s receive buffer.
	*/
	
    if (resmgr_msgread(ctp, buf, msg->i.nbytes, sizeof(msg->i)) == -1) {
        free (buf);
        return (errno);
    }

    // do something with the data
	buf [msg->i.nbytes] = '\0'; /* just in case the text is not NULL terminated */
	printf ("Received %d bytes = %s\n", msg -> i.nbytes, buf);


	offset = ocb -> attr -> base.inode - 1;

	if(atoz_attrs[offset].content != NULL) free(atoz_attrs[offset].content);
	atoz_attrs[offset].content = (char*)malloc(strlen(buf));
	strcpy(atoz_attrs[offset].content, buf);
	atoz_attrs[offset].base.nbytes = strlen(buf);


	fname = offset+'a';
	printf ("%s is writen to file /dev/atoz/%c.\n", atoz_attrs[offset].content, fname);

    // free the buffer
	free(buf);

    // if any data written, update POSIX structures
	if (msg->i.nbytes > 0)
		ocb->attr->base.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

	return (_RESMGR_NPARTS (0));
}    


static int
my_open (resmgr_context_t *ctp, io_open_t *msg, IOFUNC_ATTR_T *attr, void *extra)
{
    if (msg -> connect.path [0] == 0) {     // the directory (/dev/atoz)
        return (iofunc_open_default (ctp, msg, &attr -> base, extra));
    } else 
	if (msg -> connect.path [1] == 0 && (msg -> connect.path [0] >= 'a' && msg -> connect.path [0] <= 'z')) 
	{    // the file (/dev/atoz/[a-z])
		int offset = msg -> connect.path [0] - 'a';
        return (iofunc_open_default (ctp, msg, &atoz_attrs[offset].base, extra));
    } else {
        return (ENOENT);
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
    int     nbytes;
    int     nleft;
    struct  dirent *dp;
    char    *reply_msg;
    char    fname [_POSIX_PATH_MAX];

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

        // create the filename
        sprintf (fname, "%c", ocb -> offset + 'a');

        // see how big the result is
        nbytes = dirent_size (fname);

        // do we have room for it?
        if (nleft - nbytes >= 0) {

            // fill the dirent, and advance the dirent pointer
            dp = dirent_fill (dp, ocb -> offset + 1, ocb -> offset, fname);

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
my_read_file (resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb)
{
    int     nbytes;
    int     nleft;
    char    string;
	int		fileoffset;

    // we don't do any xtypes here...
    if ((msg -> i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return (ENOSYS);
    }

    // figure out how many bytes are left
    nleft = ocb -> attr ->base.nbytes - ocb -> offset;

    // and how many we can return to the client
    nbytes = min (nleft, msg -> i.nbytes);

    if (nbytes) {

		// create the output string
		fileoffset = ocb -> attr -> base.inode - 1;

        // return it to the client
        MsgReply (ctp -> rcvid, nbytes, atoz_attrs[fileoffset].content + ocb -> offset, nbytes);

        // update flags and offset
        ocb -> attr -> base.flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;
        ocb -> offset += nbytes;
    } else {
        // nothing to return, indicate End Of File
        MsgReply (ctp -> rcvid, EOK, NULL, 0);
    }

    // already done the reply ourselves
    return (_RESMGR_NOREPLY);
}

static int
my_read (resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb)
{
    int     sts;

    // use the helper function to decide if valid
    if ((sts = iofunc_read_verify (ctp, msg, ocb, NULL)) != EOK) {
        return (sts);
    }

    // decide if we should perform the "file" or "dir" read
    if (S_ISDIR (ocb -> attr ->base.mode)) {
        return (my_read_dir (ctp, msg, ocb));
    } else if (S_ISREG (ocb -> attr ->base.mode)) {
        return (my_read_file (ctp, msg, ocb));
    } else {
        return (EBADF);
    }
}

int
main (int argc, char **argv)
{
    dispatch_t              *dpp;
    resmgr_attr_t           resmgr_attr;
    resmgr_context_t        *ctp;
    resmgr_connect_funcs_t  connect_func;
    resmgr_io_funcs_t       io_func;
    IOFUNC_ATTR_T           attr;
    int                     i;

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
    attr.base.inode = NUM_ENTS + 1;      // 1-26 are reserved for 'a' through 'z' files
    attr.base.nbytes = NUM_ENTS;         // 26 entries contained in this directory

    // and for the "a" through "z" names
    for (i = 0; i < NUM_ENTS; i++) {
        iofunc_attr_init (&atoz_attrs [i].base, S_IFREG | 0444, 0, 0);
        atoz_attrs [i].base.inode = i + 1;
        atoz_attrs [i].base.nbytes = 0;
    }

    // add our functions; we're only interested in io_open and io_read
    connect_func.open = my_open;
    io_func.read = my_read;
	io_func.write = my_write;

    // establish a name in the pathname space
    if (resmgr_attach (dpp, &resmgr_attr, "/dev/atoz", _FTYPE_ANY, _RESMGR_FLAG_DIR, &connect_func, &io_func, &attr) == -1) {
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
