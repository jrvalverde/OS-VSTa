head	1.9;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.9
date	94.06.21.20.58.35;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.05.30.21.28.41;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.10.19.54.29;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.08.20.04.21;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.02.28.22.04.13;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.47.08;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.07.09.18.33.25;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.10.20.31.19;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.39.06;	author vandys;	state Exp;
branches;
next	;


desc
@Main loop and miscellany

@


1.9
log
@Convert to openlog()
@
text
@/*
 * Filename:	main.c
 * Developed:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Originated:	Andy Valencia
 * Last Update: 11th May 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description: Main message handling and startup routines for the bfs
 *		filesystem (boot file system)
 */


#include <fcntl.h>
#include <fdl.h>
#include <hash.h>
#include <mnttab.h>
#include <std.h>
#include <stdio.h>
#include <sys/namer.h>
#include <sys/perm.h>
#include <syslog.h>
#include "bfs.h"


int blkdev;			/* Device this FS is mounted upon */
port_t rootport;		/* Port we receive contacts through */
struct super *sblock;		/* Our filesystem's superblock */
void *shandle;			/*  ...handle for the block entry */
static struct hash *filehash;	/* Handle->filehandle mapping */


extern valid_fname(char *, int);
extern port_t path_open(char *, int);


/*
 * Protection for all BFS files: everybody can read, only
 * group 1.1 (sys.sys) can write.
 */
static struct prot bfs_prot = {
	2,
	ACC_READ|ACC_EXEC,
	{1, 1},
	{0, ACC_WRITE}
};


/*
 * bfs_seek()
 *	Set file position
 */
static void
bfs_seek(struct msg *m, struct file *f)
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
	uperms = perm_calc(perms, nperms, &bfs_prot);
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
	f->f_inode = ino_find(ROOTINODE);
	ino_ref(f->f_inode);
	f->f_pos = 0L;
	f->f_write = (uperms & ACC_WRITE);

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
	f->f_inode = fold->f_inode;
	f->f_pos = fold->f_pos;
	f->f_write = fold->f_write;
	if (f->f_inode->i_num != ROOTINODE)
		ino_ref(f->f_inode);

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
	bfs_close(f);
	free(f);
}


/*
 * bfs_main()
 *	Endless loop to receive and serve requests
 */
static void
bfs_main()
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
		syslog(LOG_ERR, "msg_receive");
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
	case M_CONNECT :	/* New client */
		new_client(&msg);
		break;
		
	case M_DISCONNECT :	/* Client done */
		dead_client(&msg, f);
		break;
		
	case M_DUP :		/* File handle dup during exec() */
		dup_client(&msg, f);
		break;

	case M_ABORT :		/* Aborted operation */
		/*
		 * Clear any pending renames
		 */
		if (f->f_rename_id) {
			cancel_rename(f);
		}

		/*
		 * We're synchronous, so presumably everything else
		 * is all done and this abort is old news.
		 */
		msg_reply(msg.m_sender, &msg);
		break;

	case FS_OPEN :		/* Look up file from directory */
		if (!valid_fname(msg.m_buf, msg.m_buflen)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		bfs_open(&msg, f);
		break;

	case FS_READ :		/* Read file */
		bfs_read(&msg, f);
		break;

	case FS_WRITE :		/* Write file */
		bfs_write(&msg, f);
		break;
		
	case FS_SEEK :		/* Set new file position */
		bfs_seek(&msg, f);
		break;
		
	case FS_REMOVE :	/* Get rid of a file */
		bfs_remove(&msg, f);
		break;
		
	case FS_STAT :		/* Tell about file */
		bfs_stat(&msg, f);
		break;
		
	case FS_RENAME :	/* Rename a file */
		bfs_rename(&msg, f);
		break;

	default :		/* Unknown */
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
	printf("Usage: bfs -p <portpath> <fsname>\n" \
	       "   or: bfs <filepath> <fsname>\n");
	exit(1);
}

/*
 * main()
 *	Startup of a boot filesystem
 *
 * A BFS instance expects to start with a command line:
 *	$ bfs <block instance> <filesystem name>
 */
void
main(int argc, char *argv[])
{
	port_t port;
	port_name fsname;
	int x;
	char *namer_name;

	/*
	 * Initialize syslog
	 */
	openlog("bfs", LOG_PID, LOG_DAEMON);

	/*
	 * Check arguments
	 */
	if (argc == 3) {
		namer_name = argv[2];

		blkdev = open(argv[1], O_RDWR);
		if (blkdev < 0) {
			syslog(LOG_ERR, "%s %s", argv[1], strerror());
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
			syslog(LOG_ERR, "couldn't connect to block device %s",
			       argv[2]);
			exit(1);
		}
		blkdev = __fd_alloc(port);
		if (blkdev < 0) {
			syslog(LOG_ERR, "%s %s", argv[2], strerror());
			exit(1);
		}
	} else {
		usage();
	}

	/*
	 * Allocate data structures we'll need
	 */
        filehash = hash_alloc(NCACHE / 4);
	if (filehash == 0) {
		syslog(LOG_ERR, "file hash allocation failed");
		exit(1);
        }

	/*
	 * Initialise the block handler
	 */
	binit();

	/*
	 * Block device is open; read in the first block and verify
	 * that it looks like a superblock.
	 */
	shandle = bget(0);
	if (!shandle) {
		syslog(LOG_ERR, "superblock read failed for %s", argv[1]);
		exit(1);
	}
	sblock = bdata(shandle);
	if (sblock->s_magic != SMAGIC) {
		syslog(LOG_ERR, "bad superblock on %s", argv[1]);
		exit(1);
	}

	/*
	 * OK, we have a superblock to work with now.  We can now initialise
	 * the directory and inode data
	 */
	ino_init();

	/*
	 * Block device looks good.  Last check is that we can register
	 * with the given name.
	 */
	rootport = msg_port((port_name)0, &fsname);
	x = namer_register(namer_name, fsname);
	if (x < 0) {
		syslog(LOG_ERR, "can't register with namer");
		exit(1);
	}

	syslog(LOG_INFO, "filesystem established");

	/*
	 * Start serving requests for the filesystem
	 */
	bfs_main();
}
@


1.8
log
@Syslog support
@
text
@a29 1
char bfs_sysmsg[7 + NAMESZ];	/* Syslog message prefix */
d211 1
a211 1
		syslog(LOG_ERR, "%s msg_receive", bfs_sysmsg);
a327 17

/*
 * create_sysmsg()
 *	Create the first part of any syslog message
 */
static void
create_sysmsg(char *namer_name)
{
	strcpy(bfs_sysmsg, "bfs (");
	strncpy(&bfs_sysmsg[5], namer_name, 16);
	if (strlen(namer_name) >= NAMESZ) {
		bfs_sysmsg[5 + NAMESZ - 1] = '\0';
	}
	strcat(bfs_sysmsg, "):");
}


d344 5
a352 1
		create_sysmsg(namer_name);
d356 1
a356 2
			syslog(LOG_ERR, "%s %s %s", bfs_sysmsg,
			       argv[1], strerror());
a361 3
		namer_name = argv[3];
		create_sysmsg(namer_name);

d365 1
d378 2
a379 3
			syslog(LOG_ERR,
			       "%s couldn't connect to block device %s",
			       bfs_sysmsg, argv[2]);
d384 1
a384 2
			syslog(LOG_ERR, "%s %s %s", bfs_sysmsg,
			       argv[2], strerror());
d396 1
a396 2
		syslog(LOG_ERR, "%s file hash allocation failed",
		       bfs_sysmsg);
d411 1
a411 2
		syslog(LOG_ERR, "%s superblock read failed for %s",
		       bfs_sysmsg, argv[1]);
d416 1
a416 2
		syslog(LOG_ERR, "%s bad superblock on %s",
		       bfs_sysmsg, argv[1]);
d433 1
a433 2
		syslog(LOG_ERR, "%s can't register with namer",
		       bfs_sysmsg);
d437 1
a437 1
	syslog(LOG_INFO, "%s filesystem established", bfs_sysmsg);
@


1.7
log
@Cleanup, add time stamp support, add rename() support
@
text
@d5 1
a5 1
 * Last Update: 8th April 1994
d30 1
d212 1
a212 1
		syslog(LOG_ERR, "bfs: msg_receive");
d324 2
a325 2
	printf("Usage is: bfs -p <portpath> <fsname>\n");
	printf(" or: bfs <filepath> <fsname>\n");
d331 16
d365 3
d370 2
a371 1
			syslog(LOG_ERR, "bfs: %s %s", argv[1], strerror());
a373 1
		namer_name = argv[2];
d377 3
d396 2
a397 2
			       "bfs: couldn't connect to block device %s",
			       argv[2]);
d402 2
a403 1
			syslog(LOG_ERR, "bfs: %s %s", argv[2], strerror());
a405 1
		namer_name = argv[3];
d415 2
a416 1
		syslog(LOG_ERR, "bfs: file hash");
d431 2
a432 1
		syslog(LOG_ERR, "bfs: superblock read");
d437 2
a438 1
		syslog(LOG_ERR, "bfs: bad superblock on %s", argv[1]);
d455 2
a456 1
		syslog(LOG_ERR, "bfs: can't register name %s", fsname);
d459 2
@


1.6
log
@Rev boot filesystem per work from Dave Hudson
@
text
@d5 1
a5 1
 * Last Update: 3rd March 1994
d31 2
a32 1
extern int valid_fname(char *, int);
d35 1
a183 2
	extern void bfs_close();

d185 3
d241 1
a241 1
	case M_CONNECT:		/* New client */
d244 2
a245 1
	case M_DISCONNECT:	/* Client done */
d248 2
a249 1
	case M_DUP:		/* File handle dup during exec() */
d252 9
a260 1
	case M_ABORT:		/* Aborted operation */
d262 1
a262 1
		 * We're synchronous, so presumably the operation
d267 2
a268 1
	case FS_OPEN:		/* Look up file from directory */
d275 2
a276 1
	case FS_READ:		/* Read file */
d279 2
a280 1
	case FS_WRITE:		/* Write file */
d283 2
a284 1
	case FS_SEEK:		/* Set new file position */
d287 2
a288 1
	case FS_REMOVE:		/* Get rid of a file */
d291 2
a292 1
	case FS_STAT:		/* Tell about file */
d295 6
a300 1
	default:		/* Unknown */
@


1.5
log
@Convert to syslog()
@
text
@d2 8
a9 2
 * main.c
 *	Main handling loop and startup
d11 4
a14 3
#include <sys/perm.h>
#include <sys/namer.h>
#include "bfs.h"
d16 2
d19 2
a20 1
#include <fcntl.h>
d22 2
d25 5
a29 11
extern void *malloc(), bfs_open(), bfs_read(), bfs_write(), *bdata(),
	*bget(), bfs_remove(), bfs_stat();
extern char *strerror();

int blkdev;		/* Device this FS is mounted upon */
port_t rootport;	/* Port we receive contacts through */
struct super		/* Our filesystem's superblock */
	*sblock;
void *shandle;		/*  ...handle for the block entry */
static struct hash	/* Handle->filehandle mapping */
	*filehash;
d31 3
d41 2
a42 2
	{1,		1},
	{0,		ACC_WRITE}
d45 1
d62 1
d99 2
a100 1
	f->f_inode = ROOTINO;
d119 1
a136 1
	extern void iref();
d155 2
a156 2
	if (f->f_inode != ROOTINO)
		iref(f->f_inode);
d174 1
d189 1
a197 1
	seg_t resid;
d212 1
d291 1
d303 2
d310 1
a310 1
 *	$ bfs <block class> <block instance> <filesystem name>
d312 1
d317 1
a317 2
	struct msg msg;
	int chan, fd, x;
d349 2
a350 1
				"BFS: couldn't connect to block device");
d366 1
a366 1
        filehash = hash_alloc(NCACHE/4);
d371 4
a375 2
	iinit();
	dir_init();
d383 1
a383 1
		syslog(LOG_ERR, "BFS: superblock read");
d388 1
a388 1
		syslog(LOG_ERR, "BFS: bad superblock on %s", argv[1]);
d393 6
d405 1
a405 1
		syslog(LOG_ERR, "BFS: can't register name");
@


1.4
log
@Source reorg
@
text
@d11 1
a11 3
#ifdef DEBUG
#include <sys/ports.h>
#endif
d195 1
a195 1
		perror("bfs: msg_receive");
d284 1
a284 1
	printf("Usage is: bfs -p <portname> <portpath> <fsname>\n");
a301 2
#ifdef DEBUG
	int scrn, kbd;
a302 6
	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
#endif
d309 1
a309 1
			perror(argv[1]);
d313 1
a313 2
	} else if (argc == 5) {
		port_name blkname;
d323 1
a323 5
			port = -1;
			blkname = namer_find(argv[2]);
			if (blkname >= 0) {
				port = msg_connect(blkname, ACC_READ|ACC_WRITE);
			}
d331 2
a332 9
			printf("BFS: couldn't connect to block device.\n");
			exit(1);
		}
		if (mountport("/mnt", port) < 0) {
			perror("/mnt");
			exit(1);
		}
		if (chdir("/mnt") < 0) {
			perror("chdir /mnt");
d335 1
a335 1
		blkdev = open(argv[3], O_RDWR);
d337 1
a337 1
			perror(argv[3]);
d340 1
a340 1
		namer_name = argv[4];
d350 1
a350 1
		perror("file hash");
d363 1
a363 1
		perror("BFS superblock");
d368 1
a368 1
		fprintf(stderr, "BFS: bad superblock on %s\n", argv[1]);
d379 1
a379 1
		fprintf(stderr, "BFS: can't register name\n");
@


1.3
log
@Boot args work
@
text
@d1 4
d6 3
a8 3
#include <namer/namer.h>
#include <bfs/bfs.h>
#include <lib/hash.h>
@


1.2
log
@Massive rework of how a boot filesystem comes up
@
text
@d299 2
a301 1
	char *namer_name;
a302 1
#ifdef DEBUG
a308 24

	/*
	 * No arguments (not even program name!)--this is a boot
	 * module.  Drop to the defaults.
	 */
	if (argc == 0) {
		static char *my_argv[6];

		my_argv[0] = "bfs";
		my_argv[1] = "-p";
		my_argv[2] = "disk/fd";
		my_argv[3] = "fd0";
		my_argv[4] = "fs/boot";
		my_argv[5] = 0;
		argv = my_argv;
		argc = 5;

		/*
		 * To let the lower-level boot servers (disk driver
		 * and namer server, in particular) to startup
		 */
		sleep(2);
	}

d321 1
d329 11
a339 4
		blkname = namer_find(argv[2]);
		if (blkname < 0) {
			perror(argv[2]);
			exit(1);
a340 1
		port = msg_connect(blkname, ACC_READ|ACC_WRITE);
d342 1
a342 1
			perror("BFS blkdev");
@


1.1
log
@Initial revision
@
text
@d6 1
d8 1
a8 1
#include <fcntl.h>
d15 1
a15 1
port_t blkdev;		/* Device this FS is mounted upon */
d88 1
a88 1
	f->f_write = m->m_arg & ACC_WRITE;
d276 11
d296 1
a296 1
	port_name blkname;
d299 10
d311 2
a312 2
	 * Our first argument should be the block device we should
	 * interpret as a filesystem.
d314 17
a330 5
	if (argc != 3) {
		fprintf(stderr,
		 "Usage is: %s <bdev> <ourpoint>\n",
			argv[0]);
		exit(1);
d332 46
a377 4
	blkname = namer_find(argv[1], 0);
	if (blkname < 0) {
		perror("BFS blkname");
		exit(1);
d379 1
a379 13
#ifndef DEBUG
	blkdev = msg_connect(blkname);
	if (blkdev < 0) {
		perror("BFS blkdev");
		exit(1);
	}
#else
	blkdev = open("tfs", O_RDWR|O_BINARY);
	if (blkdev < 0) {
		perror("tfs");
		exit(1);
	}
#endif
d411 2
a412 2
	rootport = msg_port((port_name)0);
	x = namer_register(argv[2], rootport);
@
