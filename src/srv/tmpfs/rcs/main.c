head	1.10;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5
	V1_1:1.5
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.10
date	94.11.16.19.37.51;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.06.21.20.58.58;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.05.30.21.29.10;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.05.21.21.45.26;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.02.28.22.05.50;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.49.42;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.02.23.58.29;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.07.09.18.35.09;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.29.22.58.43;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.12.20.51.08;	author vandys;	state Exp;
branches;
next	;


desc
@Main message loop
@


1.10
log
@Add FS_FID support so we can run a.out's from /tmp
@
text
@/*
 * main.c
 *	Main handling loop and startup
 */
#include <sys/namer.h>
#include "tmpfs.h"
#include <hash.h>
#include <llist.h>
#include <stdio.h>
#include <fcntl.h>
#include <std.h>
#include <syslog.h>

#define NCACHE (16)	/* Roughly, # clients */

static struct hash	/* Map of all active users */
	*filehash;
port_t rootport;	/* Port we receive contacts through */
struct llist		/* All files in filesystem */
	files;
char *zeroes;		/* BLOCKSIZE worth of 0's */

/*
 * tmpfs_seek()
 *	Set file position
 */
static void
tmpfs_seek(struct msg *m, struct file *f)
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

	/*
	 * Get data structure
	 */
	if ((f = malloc(sizeof(struct file))) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Fill in fields
	 */
	f->f_file = 0;
	f->f_pos = 0;
	f->f_nperm = nperms;
	bcopy(m->m_buf, &f->f_perms, nperms * sizeof(struct perm));
	f->f_perm = ACC_READ|ACC_WRITE;

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
	 * Fill in fields
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
	 * Add ref
	 */
	if (f->f_file) {
		f->f_file->o_refs += 1;
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
	tmpfs_close(f);
	free(f);
}

/*
 * tmpfs_main()
 *	Endless loop to receive and serve requests
 */
static void
tmpfs_main()
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
		if ((msg.m_nseg != 1) || !valid_fname(msg.m_buf,
				msg.m_buflen)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		tmpfs_open(&msg, f);
		break;

	case FS_ABSREAD:	/* Set position, then read */
		if (msg.m_arg1 < 0) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1;
		/* VVV fall into VVV */
	case FS_READ:		/* Read file */
		tmpfs_read(&msg, f);
		break;

	case FS_ABSWRITE:	/* Set position, then write */
		if (msg.m_arg1 < 0) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1;
		/* VVV fall into VVV */
	case FS_WRITE:		/* Write file */
		tmpfs_write(&msg, f);
		break;

	case FS_SEEK:		/* Set new file position */
		tmpfs_seek(&msg, f);
		break;
	case FS_REMOVE:		/* Get rid of a file */
		if ((msg.m_nseg != 1) || !valid_fname(msg.m_buf,
				msg.m_buflen)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		tmpfs_remove(&msg, f);
		break;
	case FS_STAT:		/* Tell about file */
		tmpfs_stat(&msg, f);
		break;
	case FS_WSTAT:		/* Set stuff on file */
		tmpfs_wstat(&msg, f);
		break;

	case FS_FID:		/* Return file ID for caching */
		tmpfs_fid(&msg, f);
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
	printf("Usage is: tmpfs <fsname>\n");
	exit(1);
}

/*
 * main()
 *	Startup of a tmpfs filesystem
 *
 * A TMPFS instance expects to start with a command line:
 *	$ tmpfs <filesystem name>
 */
main(int argc, char *argv[])
{
	port_name fsname;
	char *namer_name;
	int x;

	/*
	 * Initialize syslog
	 */
	openlog("tmpfs", LOG_PID, LOG_DAEMON);

	/*
	 * Check arguments
	 */
	if (argc != 2) {
		usage();
	}

	/*
	 * Name we'll offer service as
	 */
	namer_name = argv[1];

	/*
	 * Zero memory
	 */
	zeroes = malloc(BLOCKSIZE);
	if (zeroes == 0) {
		syslog(LOG_ERR, "unable to allocte zeroes");
		exit(1);
	}
	bzero(zeroes, BLOCKSIZE);

	/*
	 * Allocate data structures we'll need
	 */
        filehash = hash_alloc(NCACHE/4);
	if (filehash == 0) {
		syslog(LOG_ERR, "file hash not allocated");
		exit(1);
        }
	ll_init(&files);

	/*
	 * Last check is that we can register with the given name.
	 */
	rootport = msg_port((port_name)0, &fsname);
	x = namer_register(namer_name, fsname);
	if (x < 0) {
		syslog(LOG_ERR, "can't register name '%s'", fsname);
		exit(1);
	}

	/*
	 * Start serving requests for the filesystem
	 */
	syslog(LOG_INFO, "filesystem established");
	tmpfs_main();
}
@


1.9
log
@Convert to openlog()
@
text
@a15 3
extern void tmpfs_open(), tmpfs_read(), tmpfs_write(),
	tmpfs_remove(), tmpfs_stat(), tmpfs_close(), tmpfs_wstat();

d244 5
@


1.8
log
@Syslog support
@
text
@a24 1
char tmpfs_sysmsg[9 + NAMESZ];
d174 1
a174 1
		syslog(LOG_ERR, "%s msg_receive", tmpfs_sysmsg);
d267 1
a267 1
 *	Startup of a boot filesystem
d279 5
a295 10
	 * Sort out the syslog prefix message
	 */
	strcpy(tmpfs_sysmsg, "tmpfs (");
	strncpy(&tmpfs_sysmsg[7], namer_name, 16);
	if (strlen(namer_name) >= NAMESZ) {
		tmpfs_sysmsg[7 + NAMESZ - 1] = '\0';
	}
	strcat(tmpfs_sysmsg, "):");

	/*
d300 1
a300 1
		syslog(LOG_ERR, "%s unable to allocte zeroes", tmpfs_sysmsg);
d310 1
a310 1
		syslog(LOG_ERR, "%s file hash not allocated", tmpfs_sysmsg);
d321 1
a321 1
		syslog(LOG_ERR, "%s can't register name '%s'", tmpfs_sysmsg);
d328 1
a328 1
	syslog(LOG_INFO, "%s filesystem established", tmpfs_sysmsg);
@


1.7
log
@Forgot to initialize f_perm field
@
text
@d25 1
d175 1
a175 1
		syslog(LOG_ERR, "tmpfs: msg_receive");
d271 1
a271 1
 *	$ tmpfs <block class> <block instance> <filesystem name>
d287 15
d306 1
a306 1
		syslog(LOG_ERR, "tmpfs: zeroes");
a311 5
	 * Name we'll offer service as
	 */
	namer_name = argv[1];

	/*
d316 1
a316 1
		syslog(LOG_ERR, "tmpfs: file hash");
d327 1
a327 1
		syslog(LOG_ERR, "tmpfs: can't register name\n");
d334 1
@


1.6
log
@Convert to syslog()
@
text
@d74 1
@


1.5
log
@Source reorg
@
text
@d12 1
a12 3
#ifdef DEBUG
#include <sys/ports.h>
#endif
d173 1
a173 1
		perror("tmpfs: msg_receive");
a275 2
#ifdef DEBUG
	int scrn, kbd;
a276 7
	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
#endif

d289 1
a289 1
		perror("tmpfs: zeroes");
d304 1
a304 1
		perror("file hash");
d315 1
a315 1
		fprintf(stderr, "tmpfs: can't register name\n");
@


1.4
log
@Add access to boot keyboard/screen when DEBUG
@
text
@d1 8
a8 4
#include <namer/namer.h>
#include <tmpfs/tmpfs.h>
#include <lib/hash.h>
#include <lib/llist.h>
@


1.3
log
@Boot args work
@
text
@d8 3
d274 9
@


1.2
log
@Add abs RW support for demand-paging
@
text
@a272 15
	 * No arguments (not even program name!)--this is a boot
	 * module.  Drop to the defaults.
	 */
	if (argc == 0) {
		static char *my_argv[3];

		my_argv[0] = "tmpfs";
		my_argv[1] = "fs/tmp";
		my_argv[2] = 0;
		set_cmd("tmpfs");
		argv = my_argv;
		argc = 2;
	}

	/*
@


1.1
log
@Initial revision
@
text
@d201 8
d212 8
d223 1
@
