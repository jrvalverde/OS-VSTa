head	1.12;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.7
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.12
date	94.10.05.23.26.56;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.06.21.20.57.06;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.05.30.21.27.42;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.10.19.47.52;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.02.28.22.02.44;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.45.04;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.07.09.18.33.59;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.23.18.21.27;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.16.14.16.59;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.10.18.00.19;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.44.29;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.47.26;	author vandys;	state Exp;
branches;
next	;


desc
@Main loop
@


1.12
log
@Merge in massive fixes for the floppy driver.  It now pretty
much works!
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
#include <alloc.h>
#include <hash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/assert.h>
#include <sys/syscall.h>
#include <mach/dma.h>
#include "fd.h"


static struct hash *filehash;	/* Map session->context structure */
int fdc_type = FDC_HAVE_UNKNOWN;
				/* Type of FDC we have */
int fd_baseio = FD_BASEIO;	/* Base I/O address */
int fd_irq = FD_IRQ;		/* Interrupt request line */
int fd_dma = FD_DRQ;		/* DMA channel */
port_t fdport;			/* Port we receive contacts through */
port_name fdport_name;		/* ... its name */
char fd_name[NAMESZ] = "disk/fd";
				/* Port namer name for this server */
char fdc_names[][FDC_NAMEMAX];	/* Names of the FDC types */


/*
 * Default protection for floppy drives:  anybody can read/write, sys
 * can chmod them.
 */
struct prot fd_prot = {
	1,
	ACC_READ | ACC_WRITE,
	{1},
	{ACC_CHMOD}
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
	uperms = perm_calc(perms, nperms, &fd_prot);
	desired = m->m_arg & (ACC_WRITE | ACC_READ | ACC_CHMOD);
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
	f->f_slot = ROOTDIR;

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
	ASSERT(fold->f_list == 0, "fd dup_client: busy");
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
	fd_close(m, f);
	(void)hash_delete(filehash, m->m_sender);
	free(f);
}


/*
 * fd_main()
 *	Endless loop to receive and serve requests
 */
static void
fd_main()
{
	struct msg msg;
	int x;
	struct file *f;
	char *buf = 0;

loop:
	/*
	 * Receive a message, log an error and then keep going
	 */
	x = msg_receive(fdport, &msg);
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
		 * Hunt down any active operation for this
		 * handle, and abort it.  Then answer with
		 * an abort acknowledge.
		 */
		if (f->f_list) {
			abort_io(f);
		}
		msg_reply(msg.m_sender, &msg);
		break;

	case M_ISR:		/* Interrupt */
		ASSERT_DEBUG(f == 0, "fd: session from kernel");
		fd_isr();
		break;

	case M_TIME:		/* Time event */
		fd_time();
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

	case FS_READ:		/* Read the floppy */
	case FS_WRITE:		/* Write the floppy */
		fd_rw(&msg, f);
		break;

	case FS_STAT:		/* Get stat of file */
		fd_stat(&msg, f);
		break;

	case FS_WSTAT:		/* Writes stats */
		fd_wstat(&msg, f);
		break;

	case FS_OPEN:		/* Move from dir down into drive */
		if (!valid_fname(msg.m_buf, x)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		fd_open(&msg, f);
		break;

	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
	}
	if (buf) {
		free(buf);
		buf = 0;
	}
	goto loop;
}


/*
 * parse_options()
 *	Parse the initial command line options and establish the base I/O and
 *	IRQ settings.  Aborts if there are any missing or invalid combinations
 */
static void
parse_options(int argc, char **argv)
{
	int i;
	char *check;

	/*
	 * Start processing the option parameters
	 */
	i = 1;
	while (i < argc) {
		if (!strncmp(argv[i], "baseio=", 7)) {
			/*
			 * Select a new base I/O port address
			 */
			fd_baseio = (int)strtol(&argv[i][7], &check, 0);
			if (check == &argv[i][7] || *check != '\0') {
				fprintf(stderr, "fd: invalid I/O address " \
					"'%s' - aborting\n", argv[i]);
				exit(1);
			}
		} else if (!strncmp(argv[i], "dma=", 4)) {
			/*
			 * Select a new DMA channel
			 */
			fd_dma = (int)strtol(&argv[i][4], &check, 0);
			if (check == &argv[i][4] || *check != '\0') {
				fprintf(stderr, "fd: invalid DMA setting " \
					"'%s' - aborting\n", argv[i]);
				exit(1);
			}
		} else if (!strncmp(argv[i], "fdc=", 4)) {
			/*
			 * Force the FDC type
			 */
			for(fdc_type = 0; fdc_names[fdc_type][0];
			    fdc_type++) {
				if (!strcmp(&argv[i][4],
					    fdc_names[fdc_type])) {
					break;
				}
			}
			if (!fdc_names[fdc_type][0]) {
				fprintf(stderr, "fd: invalid FDC type " \
					"'%s' - aborting\n", argv[i]);
				exit(1);
			}
		} else if (!strncmp(argv[i], "irq=", 4)) {
			/*
			 * Select a new IRQ line
			 */
			fd_irq = (int)strtol(&argv[i][4], &check, 0);
			if (check == &argv[i][4] || *check != '\0') {
				fprintf(stderr, "fd: invalid IRQ setting " \
					"'%s' - aborting\n", argv[i]);
				exit(1);
			}
		} else if (!strncmp(argv[i], "namer=", 6)) {
			/*
			 * Select a new namer entry
			 */
			if ((strlen(&argv[i][6]) == 0)
			    || (strlen(&argv[i][6]) >= NAMESZ)) {
				fprintf(stderr, "fd: invalid name '%s' " \
					"- aborting\n", &argv[i][6]);
				exit(1);
			}
			strcpy(fd_name, &argv[i][6]);
		} else {
			fprintf(stderr,
				"fd: unknown option '%s' - aborting\n",
				argv[i]);
			exit(1);
		}
		i++;
	}
}


/*
 * main()
 *	Startup of the floppy server
 */
void
main(int argc, char **argv)
{
	/*
	 * Initialise syslog
	 */
	openlog("fd", LOG_PID, LOG_DAEMON);

	/*
	 * Work out which I/O addresses to use, etc
	 */
	parse_options(argc, argv);

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
	 * We still have to program our own DMA.  This gives the
	 * system just enough information to shut down the channel
	 * on process abort.  It can also catch accidentally launching
	 * two floppy tasks.
	 */
	if (enable_dma(fd_dma) < 0) {
		syslog(LOG_ERR, "DMA %d not enabled", fd_dma);
		exit(1);
	}

	/*
	 * Enable I/O for the required range
	 */
	if (enable_io(MIN(fd_baseio + FD_LOW, DMA_LOW),
		      MAX(fd_baseio + FD_HIGH, DMA_HIGH)) < 0) {
		syslog(LOG_ERR, "I/O permissions not granted");
		exit(1);
	}

	/*
	 * Get a port for the floppy task
	 */
	fdport = msg_port((port_name)0, &fdport_name);

	/*
	 * Register as floppy drives
	 */
	if (namer_register(fd_name, fdport_name) < 0) {
		syslog(LOG_ERR, "can't register name '%s'", fd_name);
		exit(1);
	}

	/*
	 * Tell system about our I/O vector
	 */
	if (enable_isr(fdport, fd_irq)) {
		syslog(LOG_ERR, "couldn't get IRQ %d", fd_irq);
		exit(1);
	}
	
	/*
	 * Init our data structures.  We must enable_dma() first, because
	 * init wires a bounce buffer, and you have to be a DMA-type
	 * server to wire memory.  We also need the IRQ hooked as some FDCs
	 * can't handle versioning commands and respond with an invalid
	 * command interrupt.
	 */
	fd_init();

	/*
	 * Start serving requests for the filesystem
	 */
	fd_main();
}
@


1.11
log
@Convert to openlog()
@
text
@d17 1
a19 1
#define MAXBUF (32*1024)
d22 10
a32 2
port_t fdport;		/* Port we receive contacts through */
port_name fdname;	/*  ...its name */
d40 1
a40 1
	ACC_READ|ACC_WRITE,
d45 1
d63 1
a63 1
	desired = m->m_arg & (ACC_WRITE|ACC_READ|ACC_CHMOD);
d83 1
a83 1
	f->f_unit = ROOTDIR;
d100 1
d141 1
d154 1
d194 1
d198 1
d202 1
d214 1
d219 1
d253 1
d257 1
d265 1
d277 85
d367 1
a367 1
main(void)
d370 1
a370 1
	 * Initialize syslog
d375 5
d395 2
a396 2
	if (enable_dma(FD_DRQ) < 0) {
		syslog(LOG_ERR, "DMA not enabled");
d401 1
a401 1
	 * Enable I/O for the needed range
d403 2
a404 1
	if (enable_io(FD_LOW, FD_HIGH) < 0) {
a409 7
	 * Init our data structures.  We must enable_dma() first, because
	 * init wires a bounce buffer, and you have to be a DMA-type
	 * server to wire memory.
	 */
	fd_init();

	/*
d412 1
a412 1
	fdport = msg_port((port_name)0, &fdname);
d417 2
a418 2
	if (namer_register("disk/fd", fdname) < 0) {
		syslog(LOG_ERR, "can't register name 'disk/fd'");
d425 2
a426 2
	if (enable_isr(fdport, FD_IRQ)) {
		syslog(LOG_ERR, "couldn't get IRQ %d", FD_IRQ);
d429 9
@


1.10
log
@Syslog support
@
text
@a24 1
char fd_sysmsg[] = "fd (disk/fd):";
d161 1
a161 1
		syslog(LOG_ERR, "%s msg_receive", fd_sysmsg);
d265 5
d275 1
a275 1
		syslog(LOG_ERR, "%s file hash not allocated", fd_sysmsg);
d286 1
a286 1
		syslog(LOG_ERR, "%s DMA not enabled", fd_sysmsg);
d294 1
a294 1
		syslog(LOG_ERR, "%s I/O permissions not granted", fd_sysmsg);
d314 1
a314 1
		syslog(LOG_ERR, "%s can't register name 'disk/fd'", fd_sysmsg);
d322 1
a322 1
		syslog(LOG_ERR, "%s couldn't get IRQ %d", fd_sysmsg, FD_IRQ);
@


1.9
log
@Cleanup and switch to syslog
@
text
@d25 1
d162 1
a162 1
		syslog(LOG_ERR, "fd: msg_receive");
d271 1
a271 1
		syslog(LOG_ERR, "fd: file hash");
d282 1
a282 1
		syslog(LOG_ERR, "fd: DMA");
d290 1
a290 1
		syslog(LOG_ERR, "fd: I/O");
d310 1
a310 1
		syslog(LOG_ERR, "fd: can't register name\n");
d318 1
a318 1
		syslog(LOG_ERR, "fd: IRQ");
@


1.8
log
@Remove obsolete debugging
@
text
@d9 1
d12 3
d16 1
a20 4
extern void fd_rw(), abort_io(), *malloc(), fd_init(), fd_isr(),
	fd_stat(), fd_wstat(), fd_readdir(), fd_open(), fd_close();
extern char *strerror();

d111 1
a111 1
	ASSERT(fold->f_list == 0, "dup_client: busy");
d161 1
a161 1
		perror("fd: msg_receive");
d204 1
a204 1
		fd_time(msg.m_arg);
d261 2
a262 1
main()
d270 1
a270 1
		perror("file hash");
d281 9
a289 1
		perror("Floppy DMA");
a300 8
	 * Enable I/O for the needed range
	 */
	if (enable_io(FD_LOW, FD_HIGH) < 0) {
		perror("Floppy I/O");
		exit(1);
	}

	/*
d309 1
a309 1
		fprintf(stderr, "FD: can't register name\n");
d317 1
a317 1
		perror("Floppy IRQ");
@


1.7
log
@Source reorg
@
text
@a10 1
#include <sys/ports.h>
a261 10
#ifdef DEBUG
	int scrn, kbd;

	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
#endif

@


1.6
log
@Boot args work
@
text
@d8 2
a9 2
#include <namer/namer.h>
#include <lib/hash.h>
a10 1
#include <fd/fd.h>
d13 1
@


1.5
log
@Twiddle handling of file position settings.  absread/write
position is in arg1 now
@
text
@d263 1
d271 1
@


1.4
log
@Don't trust those ?:'s
@
text
@a207 2
	case FS_ABSREAD:	/* Set position, then read */
	case FS_ABSWRITE:	/* Set position, then write */
d213 8
a220 3
		if (msg.m_op == FS_SEEK) {
			msg.m_arg = msg.m_arg1 = msg.m_nseg = 0;
			msg_reply(msg.m_sender, &msg);
d223 1
@


1.3
log
@Add SEEK support
@
text
@d220 1
a220 1
		msg.m_op = (msg.m_op == FS_ABSREAD) ? FS_READ : FS_WRITE;
@


1.2
log
@Tidy up main, get name registry working
@
text
@d207 1
d215 5
@


1.1
log
@Initial revision
@
text
@d12 1
d23 2
a24 1
port_t fdport;	/* Port we receive contacts through */
d166 2
a167 2
	 * If it's not a read/write, the data *should* fit in the
	 * single buffer.
d169 3
a171 5
	if ((x > 0) && !FS_RW(msg.m_op)) {
		if (msg.m_nseg > 0) {
			msg_err(msg.m_sender, EINVAL);
			goto loop;
		}
d249 1
a249 1
 *	Startup of the keyboard server
d253 8
d272 14
a285 1
	 * Init our data structures
a297 11
	 * We still have to program our own DMA.  This gives the
	 * system just enough information to shut down the channel
	 * on process abort.  It can also catch accidentally launching
	 * two floppy tasks.
	 */
	if (enable_dma(FD_DRQ) < 0) {
		perror("Floppy DMA");
		exit(1);
	}

	/*
d300 1
a300 1
	fdport = msg_port((port_name)0);
d305 1
a305 1
	if (namer_register("disk/fd", fdport) < 0) {
@
