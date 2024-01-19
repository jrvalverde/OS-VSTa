head	1.13;
access;
symbols
	V1_3_1:1.10
	V1_3:1.10
	V1_2:1.8
	V1_1:1.8
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.13
date	94.07.10.18.50.02;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.06.21.20.58.35;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.05.30.21.28.28;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.03.23.21.57.43;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.02.28.22.04.33;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.48.09;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.07.09.18.35.38;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.19.21.43.14;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.07.21.18.16;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.22.23.21.38;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.23.18.26.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.21.43.09;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Main routine and message handler
@


1.13
log
@Add setting of mtime for DOS files
@
text
@/*
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
@


1.12
log
@Convert to openlog()
@
text
@d26 1
a26 1
 * group 1.2 (sys.sys) can write.
d28 1
a28 1
static struct prot dos_prot = {
d32 1
a32 1
	{0,		ACC_WRITE}
d283 3
@


1.11
log
@Syslog and time support
@
text
@a19 1
char dos_sysmsg[7 + NAMESZ];	/* Syslog message prefix */
a317 15
 * create_sysmsg()
 *	Create the first part of any syslog message
 */
static void
create_sysmsg(char *namer_name)
{
	strcpy(dos_sysmsg, "dos (");
	strncpy(&dos_sysmsg[5], namer_name, 16);
	if (strlen(namer_name) >= NAMESZ) {
		dos_sysmsg[5 + NAMESZ - 1] = '\0';
	}
	strcat(dos_sysmsg, "):");
}

/*
d333 5
a341 2
		create_sysmsg(namer_name);
		
d344 1
a344 2
			syslog(LOG_ERR, "%s unable to open '%s'",
				dos_sysmsg, argv[1]);
a349 3
		namer_name = argv[3];
		create_sysmsg(namer_name);

d353 1
d366 1
a366 3
			syslog(LOG_ERR,
				"%s couldn't connect to block device",
				dos_sysmsg);
d397 1
a397 1
		syslog(LOG_ERR, "%s can't read dos boot block", dos_sysmsg);
d409 1
a409 2
		syslog(LOG_ERR, "%s can't register name: %s\n",
			dos_sysmsg, namer_name);
d423 1
a423 1
	syslog(LOG_INFO, "%s filesystem established", dos_sysmsg);
@


1.10
log
@Fix -Wall warnings, add rename() support
@
text
@d15 6
a20 6
int blkdev;		/* Device this FS is mounted upon */
port_t rootport;	/* Port we receive contacts through */
char *secbuf;		/* A sector buffer */
struct boot bootb;	/* Image of boot sector */
static struct hash	/* Handle->filehandle mapping */
	*filehash;
d313 2
a314 2
	printf("Usage is: dos -p <portpath> <fsname>\n");
	printf(" or: dos <filepath> <fsname>\n");
d317 16
d335 1
a335 1
 *	Startup of a boot filesystem
d352 3
d357 2
a358 1
			perror(argv[1]);
a360 1
		namer_name = argv[2];
d364 3
d383 2
a384 1
				"DOS: couldn't connect to block device.\n");
a391 1
		namer_name = argv[3];
d415 1
a415 1
		syslog(LOG_ERR, "Can't read dos boot block\n");
d427 2
a428 1
		syslog(LOG_ERR, "DOS: can't register name: %s\n", namer_name);
d442 1
@


1.9
log
@Convert to syslog()
@
text
@a14 4
extern void fat_init(), binit(), dir_init();
extern void dos_open(), dos_read(), dos_write(), *bdata(),
	*bget(), dos_remove(), dos_stat(), dos_fid();

d22 3
a168 2
	extern void dos_close();

d170 3
a184 1
	seg_t resid;
d235 8
a242 2
		 * We're synchronous, so presumably the operation
		 * is all done and this abort is old news.
d288 3
d324 1
d329 2
a330 3
	struct msg msg;
	int chan, fd, x;
	char *namer_name;
d420 1
@


1.8
log
@Source reorg
@
text
@d13 1
a13 3
#ifdef DEBUG
#include <sys/ports.h>
#endif
d305 1
a305 1
	printf("Usage is: dos -p <portname> <portpath> <fsname>\n");
a322 2
#ifdef DEBUG
	int scrn, kbd;
a323 6
	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
#endif
d334 1
a334 2
	} else if (argc == 5) {
		port_name blkname;
d344 1
a344 5
			port = -1;
			blkname = namer_find(argv[2]);
			if (blkname >= 0) {
				port = msg_connect(blkname, ACC_READ|ACC_WRITE);
			}
d352 2
a353 9
			printf("DOS: couldn't connect to block device.\n");
			exit(1);
		}
		if (mountport("/mnt", port) < 0) {
			perror("/mnt");
			exit(1);
		}
		if (chdir("/mnt") < 0) {
			perror("chdir /mnt");
d356 1
a356 1
		blkdev = open(argv[3], O_RDWR);
d358 1
a358 1
			perror(argv[3]);
d361 1
a361 1
		namer_name = argv[4];
d385 1
a385 1
		printf("Can't read dos boot block\n");
d397 1
a397 1
		fprintf(stderr, "DOS: can't register name: %s\n", namer_name);
@


1.7
log
@Boot args work
@
text
@d5 1
a5 1
#include <dos/dos.h>
d8 2
a9 2
#include <namer/namer.h>
#include <lib/hash.h>
@


1.6
log
@Add FID support
@
text
@d323 3
a325 1
	int chan, fd, x, bootmod = 0;
a326 1
	char *namer_name;
a327 1
#ifdef DEBUG
a333 28

	/*
	 * No arguments (not even program name!)--this is a boot
	 * module.  Drop to the defaults.
	 */
	if (argc == 0) {
		static char *my_argv[6];

		(void)set_cmd("dos");
		my_argv[0] = "dos";
		my_argv[1] = "-p";
		my_argv[2] = "disk/wd";
		my_argv[3] = "wd0_dos0";
		/* my_argv[2] = "disk/fd";
		my_argv[3] = "fd0"; */
		my_argv[4] = "fs/dos";
		my_argv[5] = 0;
		argv = my_argv;
		argc = 5;
		bootmod = 1;

		/*
		 * To let the lower-level boot servers (disk driver
		 * and namer server, in particular) to startup
		 */
		sleep(2);
	}

d346 1
d354 11
a364 4
		blkname = namer_find(argv[2]);
		if (blkname < 0) {
			perror(argv[2]);
			exit(1);
a365 1
		port = msg_connect(blkname, ACC_READ|ACC_WRITE);
d367 1
a367 1
			perror("DOS blkdev");
a420 7
	}

	/*
	 * Boot module--also register as root filesystem
	 */
	if (bootmod) {
		(void)namer_register("fs/root", fsname);
@


1.5
log
@Set command name when boot module
@
text
@d19 1
a19 1
	*bget(), dos_remove(), dos_stat();
d281 3
@


1.4
log
@If boot module, register as root filesystem as well as a
DOS filesystem.
@
text
@d339 1
@


1.3
log
@Switch back to "wd" for disk, add support for absolute read/write,
needed for demand-load executables.
@
text
@d320 1
a320 1
	int chan, fd, x;
d349 1
d436 1
a436 1
		fprintf(stderr, "DOS: can't register name\n");
d438 7
@


1.2
log
@Make write sys.sys for now (that's what the testsh is).
Also add a central routine for flushing out the varius
cached things which can get dirty.
@
text
@d250 8
d261 8
d272 1
d341 4
a344 4
		/* my_argv[2] = "disk/wd";
		my_argv[3] = "wd0_dos0"; */
		my_argv[2] = "disk/fd";
		my_argv[3] = "fd0";
@


1.1
log
@Initial revision
@
text
@d30 1
a30 1
 * group 1.2 (sys.dos) can write.
d35 1
a35 1
	{1,		2},
d433 12
@
