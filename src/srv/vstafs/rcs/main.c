head	1.18;
access;
symbols
	V1_3_1:1.16
	V1_3:1.16
	V1_2:1.14
	V1_1:1.14;
locks; strict;
comment	@ * @;


1.18
date	94.06.21.20.59.12;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.05.30.21.28.15;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.04.06.21.57.21;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.02.28.22.06.05;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.10.18.21.53.32;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.08.31.03.05.25;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.08.30.04.14.38;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.08.30.03.42.56;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.08.30.03.39.32;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.08.29.22.26.48;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.08.29.19.18.05;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.29.19.12.00;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.29.18.48.44;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.18.30.07;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.27.15.41.01;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.27.13.40.57;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.08.59.18;	author vandys;	state Exp;
branches;
next	;


desc
@Top of message processing loop, plus initialization
@


1.18
log
@Convert to openlog()
@
text
@/*
 * main.c
 *	Main loop for message processing
 */
#include "vstafs.h"
#include <sys/fs.h>
#include <sys/perm.h>
#include <sys/namer.h>
#include <hash.h>
#include <fcntl.h>
#include <std.h>
#include <stdio.h>
#include <mnttab.h>
#include <sys/assert.h>
#include <syslog.h>

extern int valid_fname(char *, int);

int blkdev;		/* Device this FS is mounted upon */
port_t rootport;	/* Port we receive contacts through */
static struct hash	/* Handle->filehandle mapping */
	*filehash;

/*
 * This "open" file just sits around as an easy way to talk about
 * the root filesystem.
 */
static struct openfile *rootdir;

/*
 * vfs_seek()
 *	Set file position
 */
static void
vfs_seek(struct msg *m, struct file *f)
{
	if (m->m_arg < 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	f->f_pos = m->m_arg+OFF_DATA;
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
	int nperms;
	struct fs_file *fs;

	/*
	 * Access dope on root dir
	 */
	fs = getfs(rootdir, 0);
	if (!fs) {
		msg_err(m->m_sender, ENOMEM);
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
	 * Fill in fields.
	 */
	perms = (struct perm *)m->m_buf;
	nperms = (m->m_buflen)/sizeof(struct perm);
	f->f_file = rootdir;
	f->f_pos = OFF_DATA;
	bcopy(perms, f->f_perms, nperms * sizeof(struct perm));
	f->f_nperm = nperms;
	f->f_rename_id = 0;

	/*
	 * Calculate perms on root dir
	 */
	f->f_perm = perm_calc(f->f_perms, f->f_nperm, &fs->fs_prot);

	/*
	 * Hash under the sender's handle
	 */
        if (hash_insert(filehash, m->m_sender, f)) {
		free(f);
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * It's a win, so ref the directory node
	 */
	ref_node(rootdir);

	/*
	 * Return acceptance
	 */
	msg_accept(m->m_sender);
}

/*
 * dup_client()
 *	Duplicate current file access onto new session
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
	*f = *fold;

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
	ref_node(f->f_file);
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
	extern void vfs_close();

	ASSERT(hash_delete(filehash, m->m_sender) == 0,
		"dead_client: mismatch");
	if (f->f_rename_id) {
		cancel_rename(f);
	}
	vfs_close(f);
	free(f);
}

/*
 * vfs_main()
 *	Endless loop to receive and serve requests
 */
static void
vfs_main()
{
	struct msg msg;
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
		 * We're synchronous, so presumably the operation
		 * is all done and this abort is old news.
		 */
		msg_reply(msg.m_sender, &msg);
		break;
	case FS_OPEN:		/* Look up file from directory */
		if (!valid_fname(msg.m_buf, msg.m_buflen)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		vfs_open(&msg, f);
		break;

	case FS_ABSREAD:	/* Set position, then read */
		if (msg.m_arg1 < 0) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1+OFF_DATA;
		/* VVV fall into VVV */
	case FS_READ:		/* Read file */
		vfs_read(&msg, f);
		break;

	case FS_ABSWRITE:	/* Set position, then write */
		if (msg.m_arg1 < 0) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1+OFF_DATA;
		/* VVV fall into VVV */
	case FS_WRITE:		/* Write file */
		vfs_write(&msg, f);
		break;

	case FS_SEEK:		/* Set new file position */
		vfs_seek(&msg, f);
		break;
	case FS_REMOVE:		/* Get rid of a file */
		vfs_remove(&msg, f);
		break;
	case FS_STAT:		/* Tell about file */
		vfs_stat(&msg, f);
		break;
	case FS_WSTAT:		/* Modify file */
		vfs_wstat(&msg, f);
		break;
	case FS_FID:		/* File ID */
		vfs_fid(&msg, f);
		break;
	case FS_RENAME:		/* Rename file */
		vfs_rename(&msg, f);
		break;
	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
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
	printf("Usage is: vfs -p <portpath> <fsname>\n");
	printf(" or: vfs <filepath> <fsname>\n");
	exit(1);
}

/*
 * verify_root()
 *	Read in root sector and apply a sanity check
 *
 * Exits on error.
 */
static void
verify_root(void)
{
	struct fs *fsroot;
	char *secbuf;

	/*
	 * Block device is open; read in the first block and verify
	 * that it looks like a superblock.
	 */
	secbuf = malloc(SECSZ);
	if (secbuf == 0) {
		syslog(LOG_ERR, "secbuf not allocated");
		exit(1);
	}
	read_sec(BASE_SEC, secbuf);
	fsroot = (struct fs *)secbuf;
	if (fsroot->fs_magic != FS_MAGIC) {
		syslog(LOG_ERR, "bad magic number on filesystem");
		exit(1);
	}
	free(secbuf);
	syslog(LOG_INFO, "%ld sectors", fsroot->fs_size);
}

/*
 * main()
 *	Startup a filesystem
 */
int
main(int argc, char *argv[])
{
	int x;
	port_name fsname;
	char *namer_name;

	/*
	 * Initialize syslog
	 */
	openlog("vstafs", LOG_PID, LOG_DAEMON);

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
		port_t port;
		int retries;
		extern int __fd_alloc(port_t);
		extern port_t path_open(char *, int);

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
		syslog(LOG_ERR, "file hash not allocated");
		exit(1);
        }

	/*
	 * Apply sanity checks on filesystem
	 */
	verify_root();

	/*
	 * Register filesystem name
	 */
	rootport = msg_port((port_name)0, &fsname);
	x = namer_register(namer_name, fsname);
	if (x < 0) {
		syslog(LOG_ERR, "can't register name '%s'", namer_name);
		exit(1);
	}

	/*
	 * Init our data structures
	 */
	init_buf();
	init_node();
	init_block();

	/*
	 * Open access to the root filesystem
	 */
	rootdir = get_node(ROOT_SEC);
	ASSERT(rootdir, "VFS: can't open root");

	/*
	 * Start serving requests for the filesystem
	 */
	vfs_main();
	return(0);
}
@


1.17
log
@Syslog support
@
text
@a22 1
char vfs_sysmsg[10 + NAMESZ];
d188 1
a188 1
		syslog(LOG_ERR, "%s msg_receive", vfs_sysmsg);
a280 15
 * create_sysmsg()
 *	Create the first part of any syslog message
 */
static void
create_sysmsg(char *namer_name)
{
	strcpy(vfs_sysmsg, "vstafs (");
	strncpy(&vfs_sysmsg[8], namer_name, 16);
	if (strlen(namer_name) >= NAMESZ) {
		vfs_sysmsg[8 + NAMESZ - 1] = '\0';
	}
	strcat(vfs_sysmsg, "):");
}

/*
d298 1
a298 1
		syslog(LOG_ERR, "%s secbuf not allocated", vfs_sysmsg);
d304 1
a304 2
		syslog(LOG_ERR, "%s bad magic number on filesystem",
			vfs_sysmsg);
d308 1
a308 1
	syslog(LOG_INFO, "%s  %ld sectors", vfs_sysmsg, fsroot->fs_size);
d323 5
a331 1
		create_sysmsg(namer_name);
d334 1
a334 2
			syslog(LOG_ERR, "%s %s %s", vfs_sysmsg,
				argv[1], strerror());
a342 3
		namer_name = argv[3];
		create_sysmsg(namer_name);

d346 1
d359 1
a359 3
			syslog(LOG_ERR,
			 	"%s couldn't connect to block device",
			 	vfs_sysmsg);
d376 1
a376 1
		syslog(LOG_ERR, "%s file hash not allocated", vfs_sysmsg);
d391 1
a391 2
		syslog(LOG_ERR, "%s can't register name '%s'",
			vfs_sysmsg, namer_name);
@


1.16
log
@Add rename() support, pass 1
@
text
@d23 1
d189 1
a189 1
		perror("vfs: msg_receive");
d282 15
d314 1
a314 1
		perror("vfs: secbuf");
d320 2
a321 1
		syslog(LOG_ERR, "Bad magic number on filesystem\n");
d325 1
a325 1
	syslog(LOG_INFO, " %ld sectors", fsroot->fs_size);
d343 2
d347 2
a348 1
			perror(argv[1]);
a350 1
		namer_name = argv[2];
d357 3
d376 2
a377 1
			 "VFS: couldn't connect to block device.\n");
a384 1
		namer_name = argv[3];
d394 1
a394 1
		perror("file hash");
d408 5
a412 1
	ASSERT(x >= 0, "VFS: can't register name");
@


1.15
log
@Convert to syslog()
@
text
@d84 1
d134 1
a134 1
	bcopy(fold, f, sizeof(struct file));
d164 3
d257 3
@


1.14
log
@Source reorg
@
text
@d15 1
d268 1
a268 1
	printf("Usage is: vfs -p <portname> <portpath> <fsname>\n");
d297 1
a297 1
		printf("Bad magic number on filesystem\n");
d301 1
a301 1
	printf(" %ld sectors", fsroot->fs_size);
d325 1
a325 2
	} else if (argc == 5) {
		port_name blkname;
d328 2
d338 1
a338 5
			port = -1;
			blkname = namer_find(argv[2]);
			if (blkname >= 0) {
				port = msg_connect(blkname, ACC_READ|ACC_WRITE);
			}
d346 2
a347 9
			printf("VFS: couldn't connect to block device.\n");
			exit(1);
		}
		if (mountport("/mnt", port) < 0) {
			perror("/mnt");
			exit(1);
		}
		if (chdir("/mnt") < 0) {
			perror("chdir /mnt");
d350 1
a350 1
		blkdev = open(argv[3], O_RDWR);
d352 1
a352 1
			perror(argv[3]);
d355 1
a355 1
		namer_name = argv[4];
a371 1
	printf("vfs mount %s", namer_name);
a396 1
	printf(".\n");
@


1.13
log
@Add WSTAT support
@
text
@d5 1
a5 1
#include <vstafs/vstafs.h>
d8 2
a9 2
#include <namer/namer.h>
#include <lib/hash.h>
@


1.12
log
@Fix ref counting bug on alloc failure.  Add sanity to
hash deletion.
@
text
@d247 3
@


1.11
log
@Pop out an informational line
@
text
@a79 1
	ref_node(rootdir);
d99 5
d160 2
a161 1
	(void)hash_delete(filehash, m->m_sender);
@


1.10
log
@Remove standalone stuff--vstafs is designed to be run with
virtual memory and such.
@
text
@d292 1
d373 1
d399 1
@


1.9
log
@Fix seek to wrong sector
@
text
@a14 3
#ifdef DEBUG
#include <sys/ports.h>
#endif
a303 2
#ifdef DEBUG
	int scrn, kbd;
a304 6
	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
#endif
@


1.8
log
@Clean up -Wall warnings
@
text
@d288 1
a288 1
	read_sec(ROOT_SEC, secbuf);
@


1.7
log
@Convert to ASSERT()
@
text
@d12 2
d19 2
d57 1
a57 1
	int uperms, nperms;
d301 1
d409 1
@


1.6
log
@Convert accesses to fs_file to use the routine getfs()
@
text
@a9 1
#include <stdio.h>
d385 1
a385 4
	if (x < 0) {
		fprintf(stderr, "VFS: can't register name: %s\n", namer_name);
		exit(1);
	}
@


1.5
log
@Calculate root dir access in-line
@
text
@a5 1
#include <vstafs/buf.h>
a55 1
	struct buf *b;
d58 1
a58 1
	 * See if they're OK to access
d60 5
a64 2
	perms = (struct perm *)m->m_buf;
	nperms = (m->m_buflen)/sizeof(struct perm);
d77 2
a87 3
	b = find_buf(rootdir->o_file, rootdir->o_len);
	ASSERT(b, "new_client: lost root");
	fs = index_buf(b, 0, 1);
@


1.4
log
@Use get_node(); coordinating w. buffer cache is fine by now
@
text
@d6 1
d56 2
d81 8
a88 1
	f->f_perm = fs_perms(f->f_perms, f->f_nperm, rootdir);
@


1.3
log
@Add init calls
@
text
@d391 1
a391 1
	rootdir = alloc_node(ROOT_SEC);
@


1.2
log
@Keep OFF_DATA concept visible in f_pos; merely hide as you
interact with the user
@
text
@d385 2
@


1.1
log
@Initial revision
@
text
@d40 1
a40 1
	f->f_pos = m->m_arg;
d75 1
a75 1
	f->f_pos = 0L;
d206 1
a206 1
		f->f_pos = msg.m_arg1;
d217 1
a217 1
		f->f_pos = msg.m_arg1;
@
