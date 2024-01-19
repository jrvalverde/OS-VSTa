head	1.11;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.8
	V1_1:1.8
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.11
date	94.06.21.20.58.58;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.05.30.21.29.10;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.02.28.22.05.30;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.48.52;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.07.09.18.34.54;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.01.18.48.15;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.27.00.30.44;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.26.23.30.18;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.23.18.17.56;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.41.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.12.30;	author vandys;	state Exp;
branches;
next	;


desc
@Main loop
@


1.11
log
@Convert to openlog()
@
text
@/*
 * main.c
 *	Main function handler for swap I/O
 */
#include <sys/perm.h>
#include <sys/namer.h>
#include <sys/swap.h>
#include <hash.h>
#include <alloc.h>
#include <stdio.h>
#include <std.h>
#include <syslog.h>
#include <sys/ports.h>

extern void swap_rw(), swap_stat(), swapinit(), swap_alloc(), swap_free(),
	swap_add();

port_t rootport;	/* Port we receive contacts through */
static struct hash	/* Handle->filehandle mapping */
	*filehash;

/*
 * Protection for all SWAP files: any sys can read, only sys/sys
 * can write.
 */
static struct prot swap_prot = {
	2,
	0,
	{1,		1},
	{ACC_READ,	ACC_WRITE}
};

/*
 * swap_seek()
 *	Set file position
 */
static void
swap_seek(struct msg *m, struct file *f)
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
new_client(struct msg *m, uint len)
{
	struct file *f;
	struct perm *perms;
	int uperms, nperms;

	/*
	 * See if they're OK to access
	 */
	perms = (struct perm *)m->m_buf;
	nperms = len/sizeof(struct perm);
	uperms = perm_calc(perms, nperms, &swap_prot);
	if ((uperms & m->m_arg) != m->m_arg) {
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
	 * Fill in fields
	 */
	f->f_pos = 0L;
	f->f_perms = uperms;

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
	f->f_pos = fold->f_pos;
	f->f_perms = fold->f_perms;

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
	extern void swap_close();

	(void)hash_delete(filehash, m->m_sender);
	free(f);
}

/*
 * swap_main()
 *	Endless loop to receive and serve requests
 */
static void
swap_main()
{
	struct msg msg;
	int x;
	struct file *f;

loop:
	/*
	 * Receive a message, log an error and then keep going.  Note
	 * that since there's no buffer, we will get all data in
	 * terms of handles.
	 */
	x = msg_receive(rootport, &msg);
	if (x < 0) {
		syslog(LOG_ERR, "msg_receive");
		goto loop;
	}

	/*
	 * Has to fit in one buf
	 */
	if (msg.m_nseg > 1) {
		msg_err(msg.m_sender, EINVAL);
		goto loop;
	}

	/*
	 * Categorize by basic message operation
	 */
	f = hash_lookup(filehash, msg.m_sender);
	switch (msg.m_op) {
	case M_CONNECT:		/* New client */
		new_client(&msg, x);
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
		msg.m_nseg = msg.m_arg = msg.m_arg1 = 0;
		msg_reply(msg.m_sender, &msg);
		break;
	case FS_ABSREAD:	/* Set position, then read */
	case FS_ABSWRITE:	/* Set position, then write */
		if (!f || (msg.m_arg1 < 0)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1;

		/* VVV fall into VVV */

	case FS_READ:		/* Read swap */
	case FS_WRITE:		/* Write swap */
		swap_rw(&msg, f, x);
		break;
	case FS_SEEK:		/* Set new file position */
		swap_seek(&msg, f);
		break;
	case FS_STAT:		/* Tell about swap */
		swap_stat(&msg, f);
		break;
	case SWAP_ADD:		/* Add new swap */
		swap_add(&msg, f, x);
		break;
	case SWAP_ALLOC:	/* Allocate some swap */
		swap_alloc(&msg, f);
		break;
	case SWAP_FREE:		/* Free some swap */
		swap_free(&msg, f);
		break;
	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
	}
	goto loop;
}

/*
 * main()
 *	Startup of swap support
 */
main()
{
	/*
	 * Initialize syslog
	 */
	openlog("swap", LOG_PID, LOG_DAEMON);

	/*
	 * Allocate data structures we'll need
	 */
        filehash = hash_alloc(16);
	if (filehash == 0) {
		syslog(LOG_ERR, "file hash");
		exit(1);
        }
	swapinit();

	/*
	 * Register as THE system swap task
	 */
	rootport = msg_port(PORT_SWAP, 0);
	if (rootport < 0) {
		syslog(LOG_ERR, "can't register name");
		exit(1);
	}

	/*
	 * Start serving requests for the filesystem
	 */
	swap_main();
}
@


1.10
log
@Syslog support
@
text
@a18 1
char swap_sysmsg[] = "swap (SWAP):";
d174 1
a174 1
		syslog(LOG_ERR, "%s msg_receive", swap_sysmsg);
d251 5
d260 1
a260 1
		syslog(LOG_ERR, "%s file hash", swap_sysmsg);
d270 1
a270 1
		syslog(LOG_ERR, "%s can't register name", swap_sysmsg);
@


1.9
log
@Convert to syslog()
@
text
@d19 1
d175 1
a175 1
		syslog(LOG_ERR, "swap: msg_receive");
d256 1
a256 1
		syslog(LOG_ERR, "file hash");
d266 1
a266 1
		syslog(LOG_ERR, "SWAP: can't register name");
@


1.8
log
@Source reorg
@
text
@a9 1
#include <sys/ports.h>
d12 2
d174 1
a174 1
		perror("swap: msg_receive");
a249 9
#ifdef DEBUG
	port_t kbd, cons;

	kbd = msg_connect(PORT_KBD, ACC_READ);
	__fd_alloc(kbd);
	cons = msg_connect(PORT_CONS, ACC_WRITE);
	__fd_alloc(cons);
	__fd_alloc(cons);
#endif
d255 1
a255 1
		perror("file hash");
d265 1
a265 1
		fprintf(stderr, "SWAP: can't register name\n");
@


1.7
log
@Boot args work
@
text
@d6 4
a9 4
#include <namer/namer.h>
#include <swap/swap.h>
#include <lib/hash.h>
#include <lib/alloc.h>
@


1.6
log
@Don't map msg ops for this driver--we want to pass absolute
writes on to ultimate device.
@
text
@d250 7
a256 7
	{ port_t kbd, cons;
	  kbd = msg_connect(PORT_KBD, ACC_READ);
	  __fd_alloc(kbd);
	  cons = msg_connect(PORT_CONS, ACC_WRITE);
	  __fd_alloc(cons);
	  __fd_alloc(cons);
	}
a257 5
	/*
	 * Our name, always
	 */
	(void)set_cmd("swap");

@


1.5
log
@Fix logic error in calculating permission.  Also add call to
set our name.
@
text
@a213 1
		msg.m_op = (msg.m_op == FS_ABSREAD) ? FS_READ : FS_WRITE;
@


1.4
log
@Add DEBUG print support, check nseg correctly, get rid
of bogus external, use official prototypes.
@
text
@d65 1
a65 1
	if ((uperms & m->m_arg) != uperms) {
d248 1
a248 1
main(int argc, char *argv[])
d259 5
@


1.3
log
@Offset argument for absread/write is in arg1 now
@
text
@d12 1
a15 1
extern char *strerror();
a57 1
	char *buf;
d62 1
a62 9
	if ((buf = alloca(len)) == 0) {
		msg_err(m->m_sender, ENOMEM);
		return;
	}
	if (seg_copyin(m->m_seg, m->m_nseg, buf, len) != len) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	perms = (struct perm *)buf;
a101 9
 *
 * This is more of a Plan9 clone operation.  The intent is
 * to not share a struct file, so that when you walk it down
 * a level or seek it, you don't affect the thing you cloned
 * off from.
 *
 * This is a kernel-generated message; the m_sender is the
 * current user; m_arg specifies a handle which will be used
 * if we complete the operation with success.
a106 1
	extern void iref();
d180 2
a181 2
	if (msg.m_nseg != 1) {
		msg_error(msg.m_sender, EINVAL);
d204 1
d250 9
@


1.2
log
@New arg for msg_port
@
text
@d227 1
a227 1
		if (!f || (msg.m_arg < 0)) {
d231 1
a231 1
		f->f_pos = msg.m_arg;
@


1.1
log
@Initial revision
@
text
@d281 1
a281 1
	rootport = msg_port(PORT_SWAP);
@
