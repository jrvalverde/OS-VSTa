head	1.13;
access;
symbols
	V1_3_1:1.11
	V1_3:1.11
	V1_2:1.9
	V1_1:1.8
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.13
date	94.10.06.01.56.15;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.06.21.20.57.06;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.03.04.02.02.21;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.02.28.22.03.40;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.01.15.02.12.21;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.45.20;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.08.02.20.16.27;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.07.09.18.34.07;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.01.18.49.00;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.23.18.25.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.19.12.47.13;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.16.14.10.30;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.12.19.56.40;	author vandys;	state Exp;
branches;
next	;


desc
@Main routine
@


1.13
log
@Add support for multiple controllers and such
@
text
@/*
 * main.c
 *	Main message handling
 */
#include <sys/msg.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include <sys/namer.h>
#include <hash.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/assert.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <syslog.h>
#include "wd.h"

extern int valid_fname();

static struct hash *filehash;	/* Map session->context structure */
int wd_baseio;			/* Base I/O address */
int wd_irq;			/* IRQ number in use */
port_t wdport;			/* Port we receive contacts through */
port_name wdname;		/*  ...its name */
uint partundef;			/* Can we take clients yet? */
char *secbuf;			/* Sector-aligned buffer for bootup */
char wd_namer_name[NAMESZ];	/* Namer entry */

/*
 * Per-disk state information
 */
struct disk disks[NWD];

/*
 * Top-level protection for WD hierarchy
 */
struct prot wd_prot = {
	2,
	0,
	{1, 1},
	{ACC_READ, ACC_WRITE|ACC_CHMOD}
};

/*
 * new_client()
 *	Create new per-connect structure
 */
static void
new_client(struct msg *m)
{
	struct file *f;
	struct perm *perms;
	int uperms, nperms, desired;

	/*
	 * See if they're OK to access
	 */
	perms = (struct perm *)m->m_buf;
	nperms = (m->m_buflen)/sizeof(struct perm);
	uperms = perm_calc(perms, nperms, &wd_prot);
	desired = m->m_arg & (ACC_WRITE|ACC_READ|ACC_CHMOD);
	if ((uperms & desired) != desired) {
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
	 * Fill in fields.
	 */
	bzero(f, sizeof(*f));
	f->f_sender = m->m_sender;
	f->f_flags = uperms;
	f->f_node = ROOTDIR;

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
	 * Fill in fields.  Simply duplicate old file.
	 */
	ASSERT(fold->f_list == 0, "dup_client: busy");
	*f = *fold;
	f->f_sender = m->m_arg;

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
	m->m_arg = m->m_buflen = m->m_nseg = m->m_arg1 = 0;
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
	free(f);
}

/*
 * wd_main()
 *	Endless loop to receive and serve requests
 */
static void
wd_main(void)
{
	struct msg msg;
	int x;
	struct file *f;
loop:
	/*
	 * Receive a message, log an error and then keep going
	 */
	x = msg_receive(wdport, &msg);
	if (x < 0) {
		syslog(LOG_ERR, "msg_receive");
		goto loop;
	}

	/*
	 * Must fit in one buffer.  XXX scatter/gather might be worth
	 * the trouble for FS_RW() operations.
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
		if (partundef) {
			msg_err(msg.m_sender, EBUSY);
			break;
		}
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
		 * If active, flag operation as aborted.  If not,
		 * return abort answer immediately.
		 */
		if (f->f_list) {
			f->f_abort = 1;
		} else {
			msg.m_nseg = msg.m_arg = 0;
			msg_reply(msg.m_sender, &msg);
		}
		break;
	case M_ISR:		/* Interrupt */
		ASSERT_DEBUG(f == 0, "wd: session from kernel");
		wd_isr();
		break;

	case FS_SEEK:		/* Set position */
		if (!f || (msg.m_arg < 0)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg;
		msg.m_arg = msg.m_arg1 = msg.m_nseg = 0;
		msg_reply(msg.m_sender, &msg);
		break;

	case FS_ABSREAD:	/* Set position, then read */
	case FS_ABSWRITE:	/* Set position, then write */
		if (!f || (msg.m_arg1 < 0)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		f->f_pos = msg.m_arg1;
		msg.m_op = ((msg.m_op == FS_ABSREAD) ? FS_READ : FS_WRITE);

		/* VVV fall into VVV */

	case FS_READ:		/* Read the disk */
	case FS_WRITE:		/* Write the disk */
		wd_rw(&msg, f);
		break;

	case FS_STAT:		/* Get stat of file */
		wd_stat(&msg, f);
		break;
	case FS_WSTAT:		/* Writes stats */
		wd_wstat(&msg, f);
		break;
	case FS_OPEN:		/* Move from dir down into drive */
		if (!valid_fname(msg.m_buf, x)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		wd_open(&msg, f);
		break;
	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
	}
	goto loop;
}

/*
 * main()
 *	Startup of the WD hard disk server
 */
int
main(int argc, char **argv)
{
	int i;

	/*
	 * Initialise syslog
	 */
	openlog("wd", LOG_PID, LOG_DAEMON);

	/*
	 * Allocate handle->file hash table.  8 is just a guess
	 * as to what we'll have to handle.
	 */
        filehash = hash_alloc(8);
	if (filehash == 0) {
		syslog(LOG_ERR, "file hash not allocated");
		exit(1);
        }

	/*
	 * We don't use ISA DMA, but we *do* want to be able to
	 * copyout directly into the user's buffer if they
	 * desire.  So we flag ourselves as DMA, but don't
	 * consume a channel.
	 */
	if (enable_dma(0) < 0) {
		syslog(LOG_ERR, "DMA not enabled");
		exit(1);
	}

	/*
	 * Init our data structures.
	 */
	wd_init(argc, argv);
	rw_init();

	/*
	 * Get a port for the disk task
	 */
	wdport = msg_port((port_name)0, &wdname);

	/*
	 * Register as WD hard drive
	 */
	if (namer_register(wd_namer_name, wdname) < 0) {
		syslog(LOG_ERR, "can't register name '%s'", wd_namer_name);
		exit(1);
	}

	/*
	 * Tell system about our I/O vector
	 */
	if (enable_isr(wdport, wd_irq)) {
		syslog(LOG_ERR, "can't enable IRQ %d", wd_irq);
		exit(1);
	}

	/*
	 * Kick off I/O's to get the disk partition table entries.
	 * We will reject clients until this process is finished.
	 */
	partundef = 0;
	for (i = first_unit; i < NWD; i++) {
		partundef |= (1 << i);
	}
	rw_readpartitions(first_unit);

	/*
	 * Start serving requests for the filesystem
	 */
	wd_main();
	return(0);
}
@


1.12
log
@Convert to openlog()
@
text
@d14 1
a14 1
#include <std.h>
a17 3
extern void wd_rw(), wd_init(), rw_init(), wd_isr(),
	wd_stat(), wd_wstat(), wd_readdir(), wd_open(),
	rw_readpartitions();
d21 7
a28 6
port_t wdport;		/* Port we receive contacts through */
port_name wdname;	/*  ...its name */
uint partundef;		/* Can we take clients yet? */
char *secbuf;		/* Sector-aligned buffer for bootup */
extern uint first_unit;	/* Lowerst unit # configured */

d154 1
a154 1
wd_main()
d264 1
a264 3
main(argc, argv)
	int argc;
	char **argv;
d269 1
a269 1
	 * Initialize syslog
d279 1
a279 1
		syslog(LOG_ERR, "file hash");
d290 1
a290 9
		syslog(LOG_ERR, "DMA");
		exit(1);
	}

	/*
	 * Enable I/O for the needed range
	 */
	if (enable_io(WD_LOW, WD_HIGH) < 0) {
		syslog(LOG_ERR, "I/O");
d308 2
a309 2
	if (namer_register("disk/wd", wdname) < 0) {
		syslog(LOG_ERR, "can't register name\n");
d316 2
a317 2
	if (enable_isr(wdport, WD_IRQ)) {
		syslog(LOG_ERR, "IRQ attach");
@


1.11
log
@Convert to -ldpart
@
text
@d167 1
a167 1
		syslog(LOG_ERR, "wd: msg_receive");
d273 5
d283 1
a283 1
		syslog(LOG_ERR, "wd: file hash");
d294 1
a294 1
		syslog(LOG_ERR, "wd: DMA");
d302 1
a302 1
		syslog(LOG_ERR, "wd: I/O");
d321 1
a321 1
		syslog(LOG_ERR, "wd: can't register name\n");
d329 1
a329 1
		syslog(LOG_ERR, "wd: IRQ attach");
@


1.10
log
@Convert to syslog()
@
text
@d13 1
d19 3
a21 1
	wd_stat(), wd_wstat(), wd_readdir(), wd_open();
d27 1
a27 1
int upyet;		/* Can we take clients yet? */
d186 1
a186 1
		if (!upyet) {
d265 1
d270 2
d297 1
a297 1
		syslog(LOG_ERR, "wd: I/O enable");
d316 1
a316 1
		syslog(LOG_ERR, "WD: can't register name\n");
d329 1
a329 1
	 * Kick off I/O's to get the first sector of each disk.
d332 5
a336 8
	upyet = 0;
	secbuf = malloc(SECSZ*2);
	if (secbuf == 0) {
		syslog(LOG_ERR, "wd: sector buffer");
	}
	secbuf = (char *)roundup((ulong)secbuf, SECSZ);
	wd_io(FS_READ, (void *)(first_unit+1),
		first_unit, 0L, secbuf, 1);
d342 1
@


1.9
log
@Cleanup
@
text
@d14 1
a14 3
#ifdef DEBUG
#include <sys/ports.h>
#endif
d164 1
a164 1
		perror("wd: msg_receive");
a265 9
#ifdef DEBUG
	int scrn, kbd;

	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
#endif
d272 1
a272 1
		perror("file hash");
d283 1
a283 1
		perror("WD DMA");
d291 1
a291 1
		perror("wd I/O");
d310 1
a310 1
		fprintf(stderr, "WD: can't register name\n");
d318 1
a318 1
		perror("WD IRQ");
d329 1
a329 1
		perror("wd: sector buffer");
@


1.8
log
@Source reorg
@
text
@d302 1
a302 1
		perror("Floppy I/O");
@


1.7
log
@Pass args to wd_init() for user-specified technique
for getting disk parms.
@
text
@d8 2
a9 2
#include <namer/namer.h>
#include <lib/hash.h>
a10 1
#include <wd/wd.h>
d17 1
@


1.6
log
@Boot args work
@
text
@d264 3
a266 1
main()
d309 1
a309 1
	wd_init();
@


1.5
log
@Post name on startup
@
text
@d264 1
a264 1
main(int argc, char **argv)
d266 1
a268 1
#ifdef DEBUG
a274 8

	/*
	 * Our name, if not inherited from execv()
	 */
	if (argc == 0) {
		(void)set_cmd("wd");
	}

@


1.4
log
@Twiddle absread/write to take position in arg1
@
text
@d264 1
a264 1
main()
d275 7
@


1.3
log
@Fix protections to allow R/W access
@
text
@a214 2
	case FS_ABSREAD:	/* Set position, then read */
	case FS_ABSWRITE:	/* Set position, then write */
d220 8
a227 3
		if (msg.m_op == FS_SEEK) {
			msg.m_arg = msg.m_arg1 = msg.m_nseg = 0;
			msg_reply(msg.m_sender, &msg);
d230 1
@


1.2
log
@Don't trust those ?:'s
@
text
@d39 1
a39 1
	1,
d42 1
a42 1
	{ACC_READ, ACC_CHMOD}
@


1.1
log
@Initial revision
@
text
@d227 1
a227 1
		msg.m_op = (msg.m_op == FS_ABSREAD) ? FS_READ : FS_WRITE;
@
