/*
 * main.c
 *	Main loop for message processing
 */
#include "dos.h"
#include <sys/fs.h>
#include <sys/perm.h>
#include <sys/namer.h>
#include <hash.h>
#include <stdio.h>
#include <fcntl.h>
#include <std.h>
#include <syslog.h>

int blkdev;			/* Device this FS is mounted upon */
port_t rootport;		/* Port we receive contacts through */
char *secbuf;			/* A sector buffer */
struct boot bootb;		/* Image of boot sector */
static struct hash *filehash;	/* Handle->filehandle mapping */

extern port_t path_open(char *, int);
extern int __fd_alloc(port_t);

/*
 * Protection for all DOSFS files: everybody can read, only
 * group 1.2 (sys.sys) can write and chmod.
 */
struct prot dos_prot = {
	2,
	ACC_READ|ACC_EXEC,
	{1,		1},
	{0,		ACC_WRITE|ACC_CHMOD}
};

/*
 * dos_seek()
 *	Set file position
 */
static void
dos_seek(struct msg *m, struct file *f)
{
	if (m->m_arg < 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	f->f_pos = m->m_arg;
	m->m_buflen = m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * new_client()
 *	Create new per-connect structure
 */
static void
new_client(struct msg *m)
{
	struct file *f;
	struct perm *perms;
	int uperms, nperms;

	/*
	 * See if they're OK to access
	 */
	perms = (struct perm *)m->m_buf;
	nperms = (m->m_buflen)/sizeof(struct perm);
	uperms = perm_calc(perms, nperms, &dos_prot);
	if ((m->m_arg & ACC_WRITE) && !(uperms & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Get data structure
	 */
	if ((f = malloc(sizeof(struct file))) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Fill in fields.  Note that our buffer is the
	 * information on the permissions our client
	 * possesses.  For an M_CONNECT, the message is
	 * from the kernel, and trusted.
	 */
	f->f_node = rootdir;
	ref_node(rootdir);
	f->f_pos = 0L;
	f->f_perm = uperms;

	/*
	 * Hash under the sender's handle
	 */
        if (hash_insert(filehash, m->m_sender, f)) {
		free(f);
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Return acceptance
	 */
	msg_accept(m->m_sender);
}

/*
 * dup_client()
 *	Duplicate current file access onto new session
 *
 * This is more of a Plan9 clone operation.  The intent is
 * to not share a struct file, so that when you walk it down
 * a level or seek it, you don't affect the thing you cloned
 * off from.
 *
 * This is a kernel-generated message; the m_sender is the
 * current user; m_arg specifies a handle which will be used
 * if we complete the operation with success.
 */
static void
dup_client(struct msg *m, struct file *fold)
{
	struct file *f;
	extern void iref();

	/*
	 * Get data structure
	 */
	if ((f = malloc(sizeof(struct file))) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Fill in fields.  Note that our buffer is the
	 * information on the permissions our client
	 * possesses.  For an M_CONNECT, the message is
	 * from the kernel, and trusted.
	 */
	f->f_node = fold->f_node;
	ref_node(f->f_node);
	f->f_pos = fold->f_pos;
	f->f_perm = fold->f_perm;

	/*
	 * Hash under the sender's handle
	 */
        if (hash_insert(filehash, m->m_arg, f)) {
		free(f);
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Return acceptance
	 */
	m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * dead_client()
 *	Someone has gone away.  Free their info.
 */
static void
dead_client(struct msg *m, struct file *f)
{
	(void)hash_delete(filehash, m->m_sender);
	if (f->f_rename_id) {
		cancel_rename(f);
	}
	dos_close(f);
	free(f);
}

/*
 * dos_main()
 *	Endless loop to receive and serve requests
 */
static void
dos_main()
{
	struct msg msg;
	char *buf2 = 0;
	int x;
	struct file *f;

loop:
	/*
	 * Receive a message, log an error and then keep going
	 */
	x = msg_receive(rootport, &msg);
	if (x < 0) {
		perror("dos: msg_receive");
		goto loop;
	}

	/*
	 * If we've received more than a buffer of data, pull it in
	 * to a dynamic buffer.
	 */
	if (msg.m_nseg > 1) {
		buf2 = malloc(x);
		if (buf2 == 0) {
			msg_err(msg.m_sender, E2BIG);
			goto loop;
		}
		if (seg_copyin(msg.m_seg,
				msg.m_nseg, buf2, x) < 0) {
			msg_err(msg.m_sender, strerror());
			goto loop;
		}
		msg.m_buf = buf2;
		msg.m_buflen = x;
		msg.m_nseg = 1;
	}

	/*
	 * Categorize by basic message operation
	 */
	f = hash_lookup(filehash, msg.m_sender);
	switch (msg.m_op) {
	case M_CONNECT:		/* New client */
		new_client(&msg);
		break;
	case M_DISCONNECT:	/* Client done */
		dead_client(&msg, f);
		break;
	case M_DUP:		/* File handle dup during exec() */
		dup_client(&msg, f);
		break;
	case M_ABORT:		/* Aborted operation */
		/*
		 * Clear pending rename if any
		 */
		if (f->f_rename_id) {
			cancel_rename(f);
		}

		/*
		 * Other operations are sync, so they're done
		 */
		msg_reply(msg.m_sender, &msg);
		break;
	case FS_OPEN:		/* Look up file from directory */
		if (!valid_fname(msg.m_buf, msg.m_buflen)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		dos_open(&msg, f);
		break;

	case FS_ABSREAD:	/* Set position, then read */
		if (msg.m_arg1 < 0) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1;
		/* VVV fall into VVV */
	case FS_READ:		/* Read file */
		dos_read(&msg, f);
		break;

	case FS_ABSWRITE:	/* Set position, then write */
		if (msg.m_arg1 < 0) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1;
		/* VVV fall into VVV */
	case FS_WRITE:		/* Write file */
		dos_write(&msg, f);
		break;

	case FS_SEEK:		/* Set new file position */
		dos_seek(&msg, f);
		break;
	case FS_REMOVE:		/* Get rid of a file */
		dos_remove(&msg, f);
		break;
	case FS_STAT:		/* Tell about file */
		dos_stat(&msg, f);
		break;
	case FS_WSTAT:		/* Set file status */
		dos_wstat(&msg, f);
		break;
	case FS_FID:		/* File ID */
		dos_fid(&msg, f);
		break;
	case FS_RENAME:		/* Rename file/dir */
		dos_rename(&msg, f);
		break;
	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
	}

	/*
	 * Free dynamic storage if in use
	 */
	if (buf2) {
		free(buf2);
		buf2 = 0;
	}
	goto loop;
}

/*
 * usage()
 *	Tell how to use the thing
 */
static void
usage(void)
{
	printf("Usage: dos -p <portpath> <fsname>\n");
	printf("   or: dos <filepath> <fsname>\n");
	exit(1);
}

/*
 * main()
 *	Startup of a DOS filesystem
 *
 * A DOS instance expects to start with a command line:
 *	$ dos <block class> <block instance> <filesystem name>
 */
int
main(int argc, char *argv[])
{
	port_t port;
	port_name fsname;
	int x;
	char *namer_name = 0;

	/*
	 * Initialize syslog
	 */
	openlog("dos", LOG_PID, LOG_DAEMON);

	/*
	 * Check arguments
	 */
	if (argc == 3) {
		namer_name = argv[2];
		blkdev = open(argv[1], O_RDWR);
		if (blkdev < 0) {
			syslog(LOG_ERR, "unable to open '%s'", argv[1]);
			exit(1);
		}
	} else if (argc == 4) {
		int retries;

		/*
		 * Version of invocation where service is specified
		 */
		namer_name = argv[3];
		if (strcmp(argv[1], "-p")) {
			usage();
		}
		for (retries = 10; retries > 0; retries -= 1) {
			port = path_open(argv[2], ACC_READ|ACC_WRITE);
			if (port < 0) {
				sleep(1);
			} else {
				break;
			}
		}
		if (port < 0) {
			syslog(LOG_ERR, "couldn't connect to block device");
			exit(1);
		}
		blkdev = __fd_alloc(port);
		if (blkdev < 0) {
			perror(argv[2]);
			exit(1);
		}
	} else {
		usage();
	}

	/*
	 * Allocate data structures we'll need
	 */
        filehash = hash_alloc(NCACHE/4);
	if (filehash == 0) {
		perror("file hash");
		exit(1);
        }

	/*
	 * Block device is open; read in the first block and verify
	 * that it looks like a superblock.
	 */
	secbuf = malloc(SECSZ);
	if (secbuf == 0) {
		perror("dos: secbuf");
		exit(1);
	}
	if (read(blkdev, secbuf, SECSZ) != SECSZ) {
		syslog(LOG_ERR, "can't read dos boot block");
		exit(1);
	}
	bcopy(secbuf, &bootb, sizeof(bootb));

	/*
	 * Block device looks good.  Last check is that we can register
	 * with the given name.
	 */
	rootport = msg_port((port_name)0, &fsname);
	x = namer_register(namer_name, fsname);
	if (x < 0) {
		syslog(LOG_ERR, "can't register name: %s\n", namer_name);
		exit(1);
	}

	/*
	 * Init our data structures
	 */
	fat_init();
	binit();
	dir_init();

	/*
	 * Start serving requests for the filesystem
	 */
	syslog(LOG_INFO, "filesystem established");
	dos_main();
	return(0);
}

/*
 * sync()
 *	Flush out all the junk which can be pending
 */
void
sync(void)
{
	bsync();
	root_sync();
	fat_sync();
}
