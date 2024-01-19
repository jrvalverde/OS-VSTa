head	1.8;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.8
date	94.10.05.18.32.29;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.06.21.20.58.58;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.05.30.21.29.10;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.02.28.22.05.19;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.49.30;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.07.09.18.34.32;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.05.06.23.20.07;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.23.19.10.15;	author vandys;	state Exp;
branches;
next	;


desc
@Main routine and message loop
@


1.8
log
@Merge in Dave Hudson's bug fixes, especially "foo | less"
function.
@
text
@/*
 * main.c
 *	Main processing loop
 */
#include "pipe.h"
#include <hash.h>
#include <stdio.h>
#include <fcntl.h>
#include <std.h>
#include <sys/namer.h>
#include <sys/assert.h>
#include <syslog.h>

#define NCACHE (16)	/* Roughly, # clients */

extern void pipe_open(), pipe_read(), pipe_write(), pipe_abort(),
	pipe_stat(), pipe_close(), pipe_wstat();

static struct hash	/* Map of all active users */
	*filehash;
port_t rootport;	/* Port we receive contacts through */
struct llist		/* All files in filesystem */
	files;

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
	bzero(f, sizeof(struct file));

	/*
	 * Fill in fields
	 */
	f->f_nperm = nperms;
	bcopy(m->m_buf, &f->f_perms, nperms * sizeof(struct perm));
	f->f_perm = ACC_READ | ACC_WRITE;

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
 */
static void
dup_client(struct msg *m, struct file *fold)
{
	struct file *f;
	struct pipe *o;

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
	if (o = f->f_file) {
		o->p_refs += 1;
		ASSERT_DEBUG(o->p_refs > 0, "dup_client: overflow");
		if (f->f_perm & ACC_WRITE) {
			o->p_nwrite += 1;
			ASSERT_DEBUG(o->p_nwrite > 0, "dup_client: overflow");
		} else {
			o->p_nread += 1;
			ASSERT_DEBUG(o->p_nread > 0, "dup_client: overflow");
		}
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
	pipe_close(f);
	free(f);
}

/*
 * pipe_main()
 *	Endless loop to receive and serve requests
 */
static void
pipe_main()
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
		pipe_abort(&msg, f);
		break;
	case FS_OPEN:		/* Look up file from directory */
		if ((msg.m_nseg != 1) || !valid_fname(msg.m_buf,
				msg.m_buflen)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		pipe_open(&msg, f);
		break;
	case FS_READ:		/* Read file */
		pipe_read(&msg, f);
		break;
	case FS_WRITE:		/* Write file */
		pipe_write(&msg, f, x);
		break;
	case FS_STAT:		/* Tell about file */
		pipe_stat(&msg, f);
		break;
	case FS_WSTAT:		/* Set stuff on file */
		pipe_wstat(&msg, f);
		break;
	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
	}
	goto loop;
}

main()
{
	port_name nm;

	/*
	 * Initialize syslog
	 */
	openlog("pipe", LOG_PID, LOG_DAEMON);

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
	 * Set up port
	 */
	rootport = msg_port(0, &nm);
	if (rootport < 0) {
		syslog(LOG_ERR, "can't establish port");
		exit(1);
	}

	/*
	 * Register port name
	 */
	if (namer_register("fs/pipe", nm) < 0) {
		syslog(LOG_ERR, "unable to register name");
		exit(1);
	}

	syslog(LOG_INFO, "pipe filesystem started");

	/*
	 * Start serving requests for the filesystem
	 */
	pipe_main();
}
@


1.7
log
@Convert to openlog()
@
text
@d56 1
d113 4
a116 2
			ASSERT_DEBUG(o->p_nwrite > 0,
				"dup_client: overflow");
@


1.6
log
@Syslog support
@
text
@a23 1
char pipe_sysmsg[] = "pipe (fs/pipe):";
d153 1
a153 1
		syslog(LOG_ERR, "%s msg_receive", pipe_sysmsg);
d206 5
d215 1
a215 1
		syslog(LOG_ERR, "%s file hash not allocated", pipe_sysmsg);
d225 1
a225 1
		syslog(LOG_ERR, "%s can't establish port", pipe_sysmsg);
d233 1
a233 1
		syslog(LOG_ERR, "%s unable to register name", pipe_sysmsg);
d237 1
a237 1
	syslog(LOG_INFO, "%s pipe filesystem started", pipe_sysmsg);
@


1.5
log
@Convert to syslog()
@
text
@d24 1
d154 1
a154 1
		syslog(LOG_ERR, "pipe: msg_receive");
d211 1
a211 1
		syslog(LOG_ERR, "pipe: file hash");
d221 1
a221 1
		syslog(LOG_ERR, "pipe: port");
d229 1
a229 1
		syslog(LOG_ERR, "pipe: name");
d232 2
@


1.4
log
@Source reorg
@
text
@a5 3
#ifdef DEBUG
#include <sys/ports.h>
#endif
d12 1
d153 1
a153 1
		perror("pipe: msg_receive");
a203 2
#ifdef DEBUG
	port_t kbd, cons;
a204 6
	kbd = msg_connect(PORT_KBD, ACC_READ);
	cons = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(kbd);
	(void)__fd_alloc(cons);
	(void)__fd_alloc(cons);
#endif
d210 1
a210 1
		perror("file hash");
d220 1
a220 1
		perror("pipe: port");
d228 1
a228 1
		perror("pipe: name");
@


1.3
log
@Boot args work
@
text
@d1 5
a5 1
#include <pipe/pipe.h>
d9 1
a9 2
#include <lib/hash.h>
#include <lib/llist.h>
d13 1
a13 1
#include <namer/namer.h>
@


1.2
log
@Fiddle with reference and writer counting; they were wrong with
dup()'s of an open writer.  Also added some debug stuff to put
sanity checks on reference count values.
@
text
@d200 1
a200 1
main(int argc, char *argv[])
a202 1

d204 1
a204 2
	{
		port_t kbd, cons;
d206 5
a210 6
		kbd = msg_connect(PORT_KBD, ACC_READ);
		cons = msg_connect(PORT_CONS, ACC_WRITE);
		(void)__fd_alloc(kbd);
		(void)__fd_alloc(cons);
		(void)__fd_alloc(cons);
	}
a211 7
	/*
	 * Set name for boot task
	 */
	if (argc == 0) {
		set_cmd("pipe");
	}

@


1.1
log
@Initial revision
@
text
@d11 1
d79 1
d106 8
a113 2
	if (f->f_file) {
		f->f_file->p_refs += 1;
@
