head	1.11;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.6
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.11
date	94.11.03.01.34.47;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.10.05.23.26.56;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.06.21.20.57.06;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.05.30.21.27.42;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.10.19.47.52;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.45.04;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.19.21.39.43;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.11.21.19.43;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.10.18.07.59;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.44.50;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.47.35;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of spinup/spindown and I/O state machine
@


1.11
log
@Add optimizations using track readahead
@
text
@/*
 * rw.c
 *	Reads and writes to the floppy device
 *
 * I/O with DMA is a little crazier than your average server.  The
 * main loop has purposely arranged for us to receive the buffer
 * in terms of segments.  If the caller has been suitably careful,
 * we can then get a physical handle on the memory one sector at a
 * time and do true raw I/O.  If the memory crosses a page boundary
 * we use a bounce buffer.
 */
#include <mach/io.h>
#include <mach/dma.h>
#include <sys/fs.h>
#include <sys/assert.h>
#include <sys/param.h>
#include <sys/syscall.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "fd.h"


/*
 * Mask used to see if a physical memory address is DMAable
 */
#define ISA_DMA_MASK 0xff000000


static void unit_spinup();
static void unit_recal();
static void unit_seek();
static void unit_spindown();
static void unit_io();
static void unit_iodone();
static void failed();
static void state_machine();
static void unit_reset();
static void unit_failed();
static void media_chgtest();
static void media_probe();
static void run_queue();


extern int fd_baseio;		/* Our controller's base I/O address */
extern int fd_dma;		/* DMA channel allocated */
extern int fd_irq;		/* Interrupt request line allocated */
int fd_retries = FD_MAXERR;	/* Per-controller number of retries allowed */
int fd_messages = FDM_FAIL;	/* Set the per controller messaging level */
extern int fdc_type;		/* Our controller type */
extern port_t fdport;		/* Our server port */
extern port_name fdport_name;	/* And it's name */
struct floppy floppies[NFD];	/* Per-floppy state */
static void *bounceva, *bouncepa;
				/* Bounce buffer */
static int map_handle;		/* Handle for DMA mem mapping */
static int configed = 0;	/* Have we got all of the drive details? */
static struct file prbf;	/* Pseudo file used in autoprobe operations */


/*
 * Floppies are such crocks that the only reasonable way to
 * handle them is to encode all their handling into a state
 * machine.  Even then, it isn't very pretty.
 *
 * The following state table draws a distinction between spinning
 * up a floppy for a new user, and spinning up a floppy while the
 * device has remained open.  The hope is that if it's the same
 * user then it's the same floppy, and we might be able to save
 * ourselves some work.  For new floppies, we always start with
 * a recalibration.
 *
 * The called routines can override the next-state assignment.
 * This is used by, for instance, unit_iodone() to cause
 * a recal after I/O error.
 *
 * Because we have to run a state machine for each drive, some
 * of the states below are not actually used.  In paticular,
 * the F_READY state entries are fake because we could well be
 * running the other floppy, and thus he's taking the events,
 * not us.  But the state entries still represent our strategy.
 */
struct state states[] = {
	{F_CLOSED,	FEV_WORK,	unit_spinup,	F_SPINUP1},
	{F_SPINUP1,	FEV_TIME,	unit_reset,	F_RESET},
	{F_RESET,	FEV_INTR,	unit_recal,	F_RECAL},
	{F_RESET,	FEV_TIME,	unit_failed,	F_CLOSED},
	{F_RECAL,	FEV_INTR,	unit_seek,	F_SEEK},
	{F_RECAL,	FEV_TIME,	unit_spindown,	F_CLOSED},
	{F_SEEK,	FEV_INTR,	unit_io,	F_IO},
	{F_SEEK,	FEV_TIME,	unit_reset,	F_RESET},
	{F_IO,		FEV_INTR,	unit_iodone,	F_READY},
	{F_IO,		FEV_TIME,	unit_reset,	F_RESET},
  	{F_READY,	FEV_WORK,	unit_seek,	F_SEEK},
	{F_READY,	FEV_TIME,	0,		F_OFF},
	{F_READY,	FEV_CLOSED,	0,		F_CLOSED},
	{F_SPINUP2,	FEV_TIME,	unit_seek,	F_SEEK},
	{F_OFF,		FEV_WORK,	unit_spinup,	F_SPINUP2},
	{F_OFF,		FEV_CLOSED,	0,		F_CLOSED},
	{0}
};


/*
 * Names of the FDC types
 */
char fdc_names[][FDC_NAMEMAX] = {
	"unknown", "765A", "765B", "82077AA", NULL
};


/*
 * Different parameters for combinations of floppies/drives.  These are
 * determined by many and varied arcane methods - having looked at 3 lots
 * of other floppy code I gave up and worked out some of my own... now
 * where did I leave that diving rod?
 */
struct fdparms fdparms[] = {
	{368640, 40, 2, 9, 0,	/* 360k in 360k drive */
		0x2a, 0x50,
		XFER_250K, 0xdf,
		0x04, SECTOR_512},
	{368640, 40, 2, 9, 1,	/* 360k in 720k or 1.44M drive */
		0x2a, 0x50,
		XFER_250K, 0xdd,
		0x04, SECTOR_512},
	{368640, 40, 2, 9, 1,	/* 360k in 1.2M drive */
		0x23, 0x50,
		XFER_300K, 0xdf,
		0x06, SECTOR_512},
	{737280, 80, 2, 9, 0,	/* 720k in 720k or 1.44M drive */
		0x2a, 0x50,
		XFER_250K, 0xdd,
		0x04, SECTOR_512},
	{737280, 80, 2, 9, 0,	/* 720k in 1.2M drive */
		0x23, 0x50,
		XFER_300K, 0xdf,
		0x06, SECTOR_512},
	{1228800, 80, 2, 15, 0,	/* 1.2M in 1.2M drive */
		0x1b, 0x54,
		XFER_500K, 0xdf,
		0x06, SECTOR_512},
	{1228800, 80, 2, 15, 0,	/* 1.2M in 1.44M drive */
		0x1b, 0x54,
		XFER_500K, 0xdd,
		0x06, SECTOR_512},
	{1474560, 80, 2, 18, 0,	/* 1.44M in 1.44M drive */
		0x1b, 0x6c,
		XFER_500K, 0xdd,
		0x06, SECTOR_512},
	{1474560, 80, 2, 18, 0,	/* 1.44M in 2.88M drive */
		0x1b, 0x6c,
		PXFER_TSFR | XFER_500K, 0xad,
		0x06, SECTOR_512},
	{2949120, 80, 2, 36, 0,	/* 2.88M in 2.88M drive */
		0x1b, 0x54,
		PXFER_TSFR | XFER_1M, 0xad,
		0x06, SECTOR_512}
};


/*
 * List of parameter sets that each type of floppy can handle.  Note that
 * we define these in the order that they will be checked in an autoprobe
 * sequence, so we must have the highest track counts first, sorted by the
 * number of sides and then the number of sectors
 */
int densities[FDTYPES][DISK_DENSITIES] = {
	{DISK_360_360, -1},
	{DISK_1200_1200, DISK_720_1200, DISK_360_1200, -1},
	{DISK_720_720, DISK_360_720, -1},
	{DISK_1440_1440, DISK_1200_1440, DISK_720_720, DISK_360_720, -1},
	{DISK_2880_2880, DISK_1440_2880, DISK_1200_1440,
		DISK_720_720, DISK_360_720, -1}
};


/*
 * Busy/waiter flags
 */
struct floppy *busy = 0;	/* Which unit is running currently */
struct llist waiters;		/* Who's waiting */


/*
 * cur_tran()
 *	Return file struct for current user
 *
 * Returns 0 if there isn't a current user
 */
static struct file *
cur_tran(void)
{
	if (busy->f_prbcount != FD_NOPROBE) {
		return(&prbf);
	}
	
	if (&waiters == waiters.l_forw) {
		return(0);
	}
	return(waiters.l_forw->l_data);
}


/*
 * fdc_in()
 *	Read a byte from the FDC
 */
static int
fdc_in(void)
{
	int t_ok = 1, j;
	struct time tim, st;

	/*
	 * Establish a timeout start time
	 */
	time_get(&st);

	do {
		j = (inportb(fd_baseio + FD_STATUS)
		     & (F_MASTER | F_DIR | F_CMDBUSY));
		if (j == (F_MASTER | F_DIR | F_CMDBUSY)) {
			break;
		}
		if (j == F_MASTER) {
			if (busy->f_messages == FDM_ALL) {
				syslog(LOG_ERR, "fdc_in failed");
			}
			return(-1);
		}
		time_get(&tim);
		t_ok = ((tim.t_sec - st.t_sec) * 1000000
			+ (tim.t_usec - st.t_usec)) < IO_TIMEOUT;
	} while(t_ok);
	
	if (!t_ok) {
		if (busy->f_messages == FDM_ALL) {
			syslog(LOG_ERR, "fdc_in failed2");
		}
		return(-1);
	}
	return(inportb(fd_baseio + FD_DATA));
}


/*
 * fdc_out()
 *	Write a byte to the FDC
 */
static int
fdc_out(uchar c)
{
	int t_ok = 1, j;
	struct time tim, st;

	/*
	 * Establish a timeout start time
	 */
	time_get(&st);

	do {
		j = (inportb(fd_baseio + FD_STATUS) & (F_MASTER | F_DIR));
		if (j == F_MASTER) {
			break;
		}
		time_get(&tim);
		t_ok = ((tim.t_sec - st.t_sec) * 1000000
			+ (tim.t_usec - st.t_usec)) < IO_TIMEOUT;
	} while(t_ok);
	
	if (!t_ok) {
		if (busy->f_messages == FDM_ALL) {
			syslog(LOG_ERR, "fdc_out failed - code %02x", j);
		}
		return(-1);
	}
	outportb(fd_baseio + FD_DATA, c);
	return(0);
}


/*
 * unit()
 *	Given unit number, return pointer to unit data structure
 */
struct floppy *
unit(int u)
{
	ASSERT_DEBUG((u >= 0) && (u < NFD), "fd unit: bad unit");
	return (&floppies[u]);
}


/*
 * calc_cyl()
 *	Given current block number, generate cyl/head values
 */
static void
calc_cyl(struct file *f, struct fdparms *fp)
{
	int head;

	f->f_cyl = (f->f_blkno / (fp->f_heads * fp->f_sectors));
	head = f->f_blkno % (fp->f_heads * fp->f_sectors);
	head /= fp->f_sectors;
	f->f_head = head;
}


/*
 * queue_io()
 *	Record parameters of I/O, queue to unit
 */
static int
queue_io(struct floppy *fl, struct msg *m, struct file *f)
{
	ASSERT_DEBUG(f->f_list == 0, "fd queue_io: busy");

	/*
	 * If they didn't provide a buffer, generate one for
	 * ourselves.
	 */
	if (m->m_nseg == 0) {
		f->f_buf = malloc(m->m_arg);
		if (f->f_buf == 0) {
			msg_err(m->m_sender, ENOMEM);
			return(1);
		}
		f->f_local = 1;
	} else {
		f->f_buf = m->m_buf;
		f->f_local = 0;
	}

	f->f_count = m->m_arg;
	f->f_blkno = f->f_pos / SECSZ(fl->f_parms.f_secsize);
	calc_cyl(f, &fl->f_parms);
	f->f_dir = m->m_op;
	f->f_off = 0;
	if ((f->f_list = ll_insert(&waiters, f)) == 0) {
		if (f->f_local) {
			free(f->f_buf);
		}
		msg_err(m->m_sender, ENOMEM);
		return(1);
	}
	return(0);
}


/*
 * childproc()
 *	Code to sleep, then send a message to the parent
 */
static int child_msecs;
static void
childproc(void)
{
	static port_t selfport = 0;
	struct time t;
	struct msg m;

	/*
	 * Child waits, then sends
	 */
	if (selfport == 0) {
		selfport = msg_connect(fdport_name, ACC_WRITE);
		if (selfport < 0) {
			selfport = 0;
			exit(1);
		}
	}

	/*
	 * Wait the interval
	 */
	time_get(&t);
	t.t_usec += (child_msecs * 1000);
	while (t.t_usec > 1000000) {
		t.t_sec += 1;
		t.t_usec -= 1000000;
	}
	time_sleep(&t);

	/*
	 * Send an M_TIME message
	 */
	m.m_op = M_TIME;
	m.m_nseg = m.m_arg = m.m_arg1 = 0;
	(void)msg_send(selfport, &m);
	_exit(0);
}


/*
 * timeout()
 *	Ask for M_TIME message in requested number of milliseconds
 */
static void
timeout(int msecs)
{
	static long child;

	/*
	 * If 0, or a child, cancel child
	 */
	if (child) {
		notify(0, child, EKILL);
		child = 0;
	}
	if (msecs == 0) {
		return;
	}
	child_msecs = msecs;

	/*
	 * We launch a child thread to send our timeout message
	 */
	child = tfork(childproc);
	if (child == -1) {
		return;
	}

	/*
	 * Parent will hear on his main port
	 */
	return;
}


/*
 * motor_mask()
 *	Return mask of bits for motor register
 */
static int
motor_mask(void)
{
	uchar motmask;
	int x;

	/*
	 * One byte holds the bits for ALL the motors.  So we have
	 * to OR together the current motor states across all drives.
	 */
	for (x = 0, motmask = 0; x < NFD; ++x) {
		if (floppies[x].f_spinning) {
			motmask |= (FD_MOTMASK << x);
		}
	}
	return(motmask);
}


/*
 * motors_off()
 *	Shut off all motors
 */
static void
motors_off(void)
{
	int x;
	struct floppy *fl;

	for (x = 0; x < NFD; ++x) {
		fl = &floppies[x];
		if (fl->f_spinning) {
			fl->f_spinning = 0;
			fl->f_state = F_OFF;
		}
	}
	outportb(fd_baseio + FD_MOTOR, FD_INTR);
}


/*
 * motors()
 *	Spin up motor on appropriate drives
 */
static void
motors(void)
{
	uchar motmask;

	motmask = motor_mask() | FD_INTR;
	if (busy) {
		motmask |= (busy->f_unit & FD_UNITMASK);
	}
	outportb(fd_baseio + FD_MOTOR, motmask);
}


/*
 * unit_spinup()
 *	Spin up motor on given unit
 */
static void
unit_spinup(int old, int new)
{
	busy->f_spinning = 1;
	motors();

	/*
	 * Get a time message in a while
	 */
	timeout(MOTOR_TIME);
}


/*
 * unit_spindown()
 *	Spin down a unit after recalibration failure
 *
 * Current operation is aborted
 */
static void
unit_spindown(int old, int new)
{
	failed(EIO);
}


/*
 * unit_recal()
 *	Recalibrate a newly-opened unit
 *
 * Also reconfigure the FDC to handle the current drive's characteristics.
 * If we're using a decent FDC we also set up some time savings.
 */
static void
unit_recal(int old, int new)
{
	struct fdparms *fp = &busy->f_parms;

	ASSERT_DEBUG(busy, "fd recal: not busy");

	/*
	 * If we've just received an interrupt from a unit_reset(), sense
	 * the status to keep the FDC happy -- drive polling mode demands
	 * this!
	 */
	if (old == F_RESET) {
		int i;

		for (i = 0; i < 4; i++) {
			fdc_out(FDC_SENSEI);
			(void)fdc_in();
			(void)fdc_in();
		}
	}

	/*
	 * If this is just a tidy up after an abort we want to get on with
	 * something productive again
	 */
	if (busy->f_abort) {
		busy->f_abort = FALSE;
		busy->f_state = F_READY;
		busy->f_ranow = 0;
		run_queue();
		if (!busy) {
			timeout(3000);
		}
		return;
	}

	/*
	 * If we have an 82077, configure the FDC.  Note that we don't
	 * allow implied seeks on stretched media :-(
	 */
	if (fdc_type == FDC_HAVE_82077) {
		fdc_out(FDC_CONFIGURE);
		fdc_out(FD_CONF1);
		if (fp->f_stretch) {
			fdc_out(FD_CONF2_STRETCH);
		} else {
			fdc_out(FD_CONF2_NOSTRETCH);
		}
		fdc_out(FD_CONF3);
	}

	/*
	 * Specify HUT (head unload time), SRT (step rate time) and
	 * HLT (Head load time)
	 */
	fdc_out(FDC_SPECIFY);
	fdc_out(fp->f_spec1);
	fdc_out(fp->f_spec2);
	busy->f_cyl = -1;
	
	/*
	 * Establish the data transfer rate
	 */
	outportb(fd_baseio + FD_CTL, fp->f_rate & PXFER_MASK);
	if (fp->f_rate & PXFER_TSFR) {
		ASSERT_DEBUG(fdc_type == FDC_HAVE_82077,
			     "fd unit_recal: not enhanced FDC");

		/*
		 * Set up any perpendicular mode transfers separately from
		 * the main tramsfers
		 */
		fdc_out(FDC_PERPENDICULAR);
		if (fp->f_rate & PXFER_MASK == XFER_1M) {
			fdc_out(PXFER_1M);
		} else {
			fdc_out(PXFER_500K);
		}
	}

	/*
	 * Take a quick look at the drive change line - we need to be
	 * sure whether we have new media or not
	 */
	media_chgtest();

	/*
	 * Start recalibrate
	 */
	fdc_out(FDC_RECAL);
	fdc_out(busy->f_unit);
	timeout(4000);
}


/*
 * unit_seek()
 *	Move floppy arm to requested cylinder
 */
static void
unit_seek(int old, int new)
{
	struct file *f = cur_tran();
	uint s0, pcn;
	int cyl;

	ASSERT_DEBUG(f, "fd seek: no work");
	ASSERT_DEBUG(busy, "fd seek: not busy");

	/*
	 * If the floppy consistently errors, we will keep re-entering
	 * this state.  It thus makes a good place to put a cap on the
	 * number of errors.  Note that we have two error limits - one for
	 * normal operations and one for autoprobe operations
	 */
	if (((busy->f_errors > busy->f_retries)
	     && (busy->f_prbcount != FD_NOPROBE))
	    || ((busy->f_errors > FD_PROBEERR)
	        && (busy->f_prbcount == FD_NOPROBE))) {
		busy->f_errors = 0;
		failed(EIO);
		return;
	}

	/*
	 * Good place to check the change-line status
	 */
	media_chgtest();

	/*
	 * Sense result if we got here from a recalibrate
	 */
	if (old == F_RECAL) {
		fdc_out(FDC_SENSEI);
		s0 = fdc_in();
		pcn = fdc_in();
		if (((s0 & 0xc0) != 0) || (pcn != 0)) {
			/*
			 * This case is unfortunately all too common - some
			 * FDCs just can't cope with a recal from track 79
			 * back down to 0 in a single attempt!
			 */
			busy->f_errors++;
			busy->f_state = F_RESET;
			unit_reset(new, F_RESET);
			return;
		}
	}

	/*
	 * If we're already there or we can handle implied seeks, advance
	 * to I/O immediately
	 */
	f = cur_tran();
	cyl = f->f_cyl;
	if ((busy->f_cyl == f->f_cyl)
	    || ((fdc_type == FDC_HAVE_82077) && (!busy->f_parms.f_stretch))) {
		/*
		 * If we've detected a media change we need to clear the
		 * change-line flag down.  If we were going to do an implied
		 * seek, simply do an explicit one, otherwise seek to the
		 * "wrong" track and ensure that the error recovery method
		 * doesn't count the resultant "failure"
		 */
		if (!busy->f_chgactive) {
			busy->f_state = F_IO;
			unit_io(F_IO, F_IO);
			return;
		} 
		if (busy->f_cyl == f->f_cyl) {
			/*
			 * Set the wrong target track, but we won't count
			 * this one as an errror
			 */
			if (cyl) {
				cyl--;
			} else {
				cyl++;
			}
			busy->f_errors--;
		}
	}

	/*
	 * Send a seek command
	 */
	fdc_out(FDC_SEEK);
	fdc_out((f->f_head << 2) | busy->f_unit);
	fdc_out(cyl << busy->f_parms.f_stretch);
	busy->f_cyl = cyl;

	/*
	 * Arrange for time notification
	 */
	timeout(5000);
}


/*
 * run_queue()
 *	If there's stuff in queue, fire state machine
 *
 * If there's nothing, clears "busy"
 */
static void
run_queue(void)
{
	struct file *f;

	if ((f = cur_tran()) == 0) {
		busy = 0;
		return;
	}
	busy = unit(f->f_unit);
	state_machine(FEV_WORK);
}


/*
 * failed()
 *	Send back an I/O error to the current operation
 *
 * We also take this opportunity to shut down the drive motor and leave 
 * the floppy state as F_CLOSED
 */
static void
failed(char *errstr)
{
	struct file *f;
	
	/*
	 * If we failed during an autoprobe operation we need to make
	 * the cur_tran() details reflect the original request, not the
	 * probe request
	 */
	if (busy->f_prbcount != FD_NOPROBE) {
		busy->f_prbcount = FD_NOPROBE;
	}

	f = cur_tran();

	/*
	 * Shut down the drive motor
	 */
	busy->f_spinning = 0;
	busy->f_state = F_CLOSED;
	motors();

	/*
	 * Detach requestor and return error
	 */
	ASSERT_DEBUG(f, "fd failed: not busy");
	ll_delete(f->f_list);
	f->f_list = 0;
	msg_err(f->f_sender, errstr);

	if (f->f_local) {
		free(f->f_buf);
	}

	/*
	 * Relase any wired pages
	 */
	if (map_handle >= 0) {
		(void)page_release(map_handle);
	}

	/*
	 * Perhaps kick off some more work
	 */
	run_queue();
}


/*
 * setup_fdc()
 *	DMA is ready, kick the FDC
 */
static void
setup_fdc(struct floppy *fl, struct file *f)
{
	struct fdparms *fp = &fl->f_parms;

	fdc_out((f->f_dir == FS_READ) ? FDC_READ : FDC_WRITE);
	fdc_out((f->f_head << 2) | fl->f_unit);
	fdc_out(f->f_cyl);
	fdc_out(f->f_head);
	if (!fl->f_ranow) {
		fdc_out((f->f_blkno % fp->f_sectors) + 1);
	} else {
		fdc_out(((fl->f_rablock + fl->f_racount) % fp->f_sectors) + 1);
	}
	fdc_out(fp->f_secsize);
	fdc_out(fp->f_sectors);
	fdc_out(fp->f_gap);
	fdc_out(0xff);		/* Data length - 0xff means ignore DTL */
}


/*
 * setup_dma()
 *	Get a physical handle on the next chunk of user memory
 *
 * Configures DMA to the user's memory if possible.  If not, uses
 * a "bounce buffer" and copyout()'s after I/O.
 */
static void
setup_dma(struct file *f)
{
	ulong pa;
	uint secsz = SECSZ(busy->f_parms.f_secsize);
	void *fbuf;
	uint foff;

	/*
	 * Sort out where the I/O's going - it could be into the
	 * read-ahead buffer
	 */
	if (!busy->f_ranow) {
		fbuf = f->f_buf + f->f_off;
	} else {
		fbuf = busy->f_rabuf + (secsz * busy->f_racount);
	}

	/*
	 * First off, we need to ensure we don't keep grabbing wired pages
	 * as they're a pretty scarce resource
	 */
	if (map_handle >= 0) {
		(void)page_release(map_handle);
		map_handle = -1;
	}

	/*
	 * Second, we need to check if the next transfer will cross a
	 * page boundary - just because two pages are contiguous in virtual
	 * space doesn't mean that they are physically.  We take the easy
	 * approach and just use the bounce buffer for this case
	 */
	if ((((uint)fbuf) & (NBPG - 1)) + secsz <= NBPG) {
		/*
		 * Try for straight map-down of memory.  Use bounce buffer
		 * if we can't get the memory directly.
		 */
		map_handle = page_wire(fbuf, (void **)&pa);
		if (map_handle > 0) {
			/*
			 * Make sure that the wired page is the DMAable
			 * area of system memory.  We'll assume for now that
			 * we're going to have to live with the usual
			 * 16 MByte ISA bus limit
			 */
			if (pa & ISA_DMA_MASK) {
				page_release(map_handle);
				map_handle = -1;
			}
		}
	}
	if (map_handle < 0) {
		/*
		 * We're going to use the bounce buffer
		 */
		pa = (ulong)bouncepa;
		
		/*
		 * Are we doing a write - if we are we need to copy the
		 * user's data into the bounce buffer
		 */
		if (f->f_dir == FS_WRITE) {
			bcopy(fbuf, bounceva, secsz);
		}
	}
	
	outportb(DMA_STAT0, (f->f_dir == FS_READ) ? DMA_READ : DMA_WRITE);
	outportb(DMA_STAT1, 0);
	outportb(DMA_ADDR, pa & 0xff);
	outportb(DMA_ADDR, (pa >> 8) & 0xff);
	outportb(DMA_HIADDR, (pa >> 16) & 0xff);
	outportb(DMA_CNT, (secsz - 1) & 0xff);
	outportb(DMA_CNT, ((secsz - 1) >> 8) & 0xff);
	outportb(DMA_INIT, 2);
}


/*
 * io_complete()
 *	Reply to our clien and handle the clean up after our I/O's complete
 */
static void
io_complete(struct file *f)
{
	struct msg m;

	/*
	 * All done. Return results.  If we used a local buffer, send it
	 * back.  Otherwise we DMA'ed into his own memory, so no segment
	 * is returned.
	 */
	if (f->f_local) {
		m.m_buf = f->f_buf;
		m.m_buflen = f->f_off;
		m.m_nseg = 1;
	} else {
		m.m_nseg = 0;
	}
	m.m_arg = f->f_off;
	m.m_arg1 = 0;
	msg_reply(f->f_sender, &m);

	/*
	 * Dequeue the completed operation.  Kick the queue so
	 * that any pending operation can now take over.
	 */
	ll_delete(f->f_list);
	f->f_list = 0;
	run_queue();

	/*
	 * He has it, so free back to our pool
	 */
	if (f->f_local) {
		free(f->f_buf);
	}

	/*
	 * Leave a timer behind; if nothing comes up, this will
	 * cause the floppies to spin down.
	 */
	if (!busy) {
		timeout(3000);
	}
}


/*
 * unit_io()
 *	Fire up an I/O on a ready/spinning unit
 */
static void
unit_io(int old, int new)
{
	struct file *f = cur_tran();

	ASSERT_DEBUG(f, "fd io: not busy");
	ASSERT_DEBUG(busy, "fd io: not busy");

	/*
	 * Sense state after seek
	 */
	if (old == F_SEEK) {
		uint s0, pcn;

		fdc_out(FDC_SENSEI);
		s0 = fdc_in();
		pcn = fdc_in();

		/*
		 * Another good place to check the change line status - we
		 * have completed a seek to get here, so the change-line
		 * should now be reset.
		 */
		media_chgtest();

		/*
		 * Make sure we're where we should be - recal if we're not
		 */
		if (pcn != (f->f_cyl << busy->f_parms.f_stretch)) {
			busy->f_errors++;
			busy->f_state = F_RECAL;
			unit_recal(F_IO, F_RECAL);
			return;
		}
	}

	/*
	 * Before we actually request the data off the media check to see
	 * that we haven't already cached it
	 */
	if ((!busy->f_ranow)
	    && (f->f_dir == FS_READ)
	    && (busy->f_racount > 0)
	    && (f->f_blkno >= busy->f_rablock)
	    && (f->f_blkno < busy->f_rablock + busy->f_racount)) {
		/*
		 * OK we have cached data we can work with
		 */
		uint secsz = SECSZ(busy->f_parms.f_secsize);
		int clen = (busy->f_rablock + busy->f_racount - f->f_blkno)
			   * secsz;

		if (clen > f->f_count) {
			clen = f->f_count;
		}
		bcopy(busy->f_rabuf + ((f->f_blkno - busy->f_rablock) * secsz),
		      f->f_buf + f->f_off, clen);
		f->f_off += clen;
		f->f_pos += clen;
		f->f_blkno += (clen / secsz);
		f->f_count -= clen;
		
		/*
		 * If we have now completed the I/O, terminate the cycle.
		 * If we've still got some to go it can only really mean that
		 * the data we're after is on the next track so seek
		 * to it
		 */
		if (f->f_count == 0) {
			busy->f_state = F_READY;
			io_complete(f);
			return;
		} else {
			calc_cyl(f, &busy->f_parms);
			busy->f_state = F_SEEK;
			unit_seek(F_READY, F_SEEK);
			return;
		}
	}

	/*
	 * Setup the DMA and FDC registers
	 */
	setup_dma(f);
	setup_fdc(unit(busy->f_unit), f);
	timeout(1000);
}


/*
 * unit_iodone()
 *	Our I/O has completed
 */
static void
unit_iodone(int old, int new)
{
	uchar results[7];
	uint secsz = SECSZ(busy->f_parms.f_secsize);
	int x;
	struct file *f = cur_tran();

	ASSERT_DEBUG(busy, "fd iodone: not busy");
	ASSERT_DEBUG(f, "fd iodone: no request");

	/*
	 * Shut off timeout
	 */
	timeout(0);

	if (map_handle >= 0) {
		/*
		 * Release memory lock-down, if DMA was to user's memory
		 */
		(void)page_release(map_handle);
	}

	/*
	 * Establish the "last used" density as being what we are now!
	 */
	if (busy->f_density != DISK_AUTOPROBE) {
		busy->f_lastuseddens = busy->f_density;
	}

	/*
	 * Read status
	 */
	for (x = 0; x < sizeof(results); ++x) {
		results[x] = fdc_in();
	}

	/*
	 * Error code?
	 */
	if (results[0] & 0xd8) {
		if (busy->f_prbcount != FD_NOPROBE) {
			/*
			 * We're actually probing the media type - this one's
			 * failed, so let's try the next one - after a few
			 * attempts!
			 */
			busy->f_errors++;
			if (busy->f_errors < busy->f_retries) {
				busy->f_state = F_RECAL;
				unit_recal(F_IO, F_RECAL);
				return;
			}

			if (busy->f_messages == FDM_ALL) {
				syslog(LOG_INFO, "attempted probe for " \
					"%d byte media failed",
					busy->f_parms.f_size);
			}
			busy->f_errors = 0;
			busy->f_prbcount++;
			if (busy->f_posdens[busy->f_prbcount] == -1) {
				busy->f_parms.f_size = FD_PUNDEF;
				failed(EIO);
				return;
			}
			busy->f_parms
				= fdparms[busy->f_posdens[busy->f_prbcount]];
			prbf.f_blkno = (busy->f_parms.f_size
					/ SECSZ(busy->f_parms.f_secsize)) - 1;
			calc_cyl(&prbf, &busy->f_parms);
			busy->f_state = F_RESET;
			unit_reset(F_SPINUP1, F_RESET);
			return;
		}
		if (results[1] & FD_WRITEPROT) {
			if (busy->f_messages <= FDM_FAIL) {
				syslog(LOG_ERR, "unit %d: write protected!",
					busy->f_unit);
			}
			failed(EACCES);
			return;
		}
		if (busy->f_messages == FDM_ALL) {
			syslog(LOG_ERR, "I/O error - %02x %02x %02x " \
				"%02x %02X %02X %02x",
				results[0], results[1], results[2],
				results[3], results[4], results[5],
				results[6]);
		}
		busy->f_errors++;
		busy->f_state = F_RECAL;
		unit_recal(F_IO, F_RECAL);
		return;
	}

	if (busy->f_prbcount != FD_NOPROBE) {
		/*
		 * We've successfully just detected a media type!  Flag
		 * that we're not probing any more an kick off the real I/O
		 * that we were going to do before we started the autoprobe
		 */
		if (busy->f_messages == FDM_ALL) {
			syslog(LOG_INFO, "probe found %d byte media",
				busy->f_parms.f_size);
		}
		busy->f_lastuseddens = busy->f_posdens[busy->f_prbcount];
		busy->f_prbcount = FD_NOPROBE;

		/*
		 * Check that we don't overflow the media capacity
		 */
		if (f->f_count + f->f_pos > busy->f_parms.f_size) {
			failed(ENOSPC);
			return;
		}

		run_queue();
		return;
	}

	/*
	 * Clear error count after successful transfer
	 */
	busy->f_errors = 0;

	/*
	 * If we weren't DMAing then we need to copy the bounce buffer
	 * back into user land
	 */
	if ((map_handle < 0) && (f->f_dir == FS_READ)) {
		/*
		 * Need to bounce out to buffer
		 */
		if (!busy->f_ranow) {
			bcopy(bounceva, f->f_buf + f->f_off, secsz);
		} else {
			bcopy(bounceva,
			      busy->f_rabuf + (secsz * busy->f_racount),
			      secsz);
		}
	}

	/*
	 * Advance I/O counters.  If we have more to do in this
	 * transfer, keep our state as F_IO and start next part
	 * of transfer.
	 */
	if (!busy->f_ranow) {
		f->f_pos += secsz;
		f->f_off += secsz;
		f->f_blkno += 1;
		f->f_count -= secsz;
		if (f->f_count > 0) {
			calc_cyl(f, &busy->f_parms);
			busy->f_state = F_SEEK;
			unit_seek(F_READY, F_SEEK);
			return;
		} else if (f->f_dir == FS_READ) {
			busy->f_ranow = 1;
			busy->f_rablock = f->f_blkno;
			busy->f_racount = -1;
		}
	}

	if (busy->f_ranow) {
		busy->f_racount++;
		if ((busy->f_rablock + busy->f_racount)
		    % busy->f_parms.f_sectors) {
			busy->f_state = F_SEEK;
			unit_seek(F_READY, F_SEEK);
			return;
		} else {
			busy->f_ranow = 0;
		}
	}

	io_complete(f);
}


/*
 * state_machine()
 *	State machine for getting work done on floppy units
 *
 * This machine is invoked in response to external events like
 * timeouts and interrupts, as well as internally-generated
 * events, like aborts, retries, and completions.
 */
static void
state_machine(int event)
{
	int x;
	struct state *s;

#ifdef XXX
	printf("state_machine(%d) cur state ", event);
	if (busy) {
		printf("%d\n", busy->f_state);
		printf("  dens = %d, unit = %d, spin = %d, " \
		       "f_cyl = %d, open = %d\n",
		       busy->f_density, busy->f_unit,
		       busy->f_spinning, busy->f_cyl, busy->f_opencnt);
	} else {
		printf("<none>\n");
	}
#endif

	/*
	 * !busy is a special case; we're not driving one of our
	 * floppy drives.  Instead, we're shutting off the motors
	 * due to enough idleness.
	 */
	if (!busy) {
		if (event == FEV_TIME) {
			motors_off();
		}
		return;
	}

	/*
	 * Look up state/event pair
	 */
	for (x = 0, s = states; s->s_state; ++x, ++s) {
		if ((s->s_event == event) &&
				(s->s_state == busy->f_state)) {
			break;
		}
	}

	/*
	 * Ignore events which don't have a table entry
	 */
	if (!s->s_state) {
		return;
	}

	/*
	 * Call event function
	 */
	busy->f_state = s->s_next;
	if (s->s_fun) {
		(*(s->s_fun))(s->s_state, s->s_next);
	}
}


/*
 * fd_rw()
 *	Do I/O to the floppy
 *
 * m_arg specifies how much they want.  It must be in increments
 * of sector sizes, or we return tidings of doom and despair.
 */
void
fd_rw(struct msg *m, struct file *f)
{
	uint secsz;
	struct floppy *fl;

	/*
	 * Sanity check operations on directories
	 */
	if (m->m_op == FS_READ) {
		if (f->f_slot == ROOTDIR) {
			fd_readdir(m, f);
			return;
		}
	} else {
		/*
		 * FS_WRITE
		 */
		if ((f->f_slot == ROOTDIR) || (m->m_nseg != 1)) {
			msg_err(m->m_sender, EINVAL);
			return;
		}
	}

	/*
	 * Check size of the I/O request
	 */
	if ((m->m_arg > MAXIO) || (m->m_arg <= 0)) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	fl = unit(f->f_unit);
	ASSERT_DEBUG(fl, "fd fd_rw: bad node number");

	/*
	 * Check alignment of request (block alignment) - note we assume that
	 * in the event that we're not using a user-defined media parameter
	 * that the sector size will be 512 bytes!
	 */
	if ((f->f_slot == SPECIALNODE)
	    && (fl->f_specialdens == DISK_USERDEF)) {
		secsz = fl->f_userp.f_secsize;
	} else {
		secsz = SECSZ(SECTOR_512);
	}

	if ((m->m_arg & (secsz - 1)) || (f->f_pos & (secsz - 1))) {
		msg_err(m->m_sender, EBALIGN);
		return;
	}

	/*
	 * Unit present?
	 */
	if (fl->f_state == F_NXIO) {
		msg_err(m->m_sender, ENXIO);
		return;
	}

	/*
	 * Check permission
	 */
	if (((m->m_op == FS_READ) && !(f->f_flags & ACC_READ)) ||
			((m->m_op == FS_WRITE) && !(f->f_flags & ACC_WRITE))) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Check that we have valid media parameters
	 */
	if (fl->f_parms.f_size == FD_PUNDEF) {
		busy = fl;
		media_probe(f);
	} else {
		/*
		 * Check that we don't overflow the media capacity
		 */
		if (m->m_arg + f->f_pos > fl->f_parms.f_size) {
			msg_err(m->m_sender, ENOSPC);
			return;
		}
	}

	/*
	 * Queue I/O to unit - note that if we started an autoprobe, or
	 * there's already a device request being performed we just queue
	 * the request, and don't attempt to kick off the state machine
	 */
	if (!queue_io(fl, m, f) && !busy) {
		busy = fl;
		state_machine(FEV_WORK);
	}
}


/*
 * fd_init()
 *	Initialise the floppy server's variable space
 *
 * Establish bounce buffers for misaligned I/O's.  Build command queues.
 * Determine the drives connected and the type of controller that we have 
 */
void
fd_init(void)
{
	int x, c;
	struct floppy *fl;
	uchar types;

	/*
	 * Get a bounce buffer for misaligned I/O
	 */
	bounceva = malloc(NBPG);
	if (page_wire(bounceva, &bouncepa) < 0) {
		syslog(LOG_ERR, "can't get bounce buffer");
		exit(1);
	}
	if ((uint)bouncepa & ISA_DMA_MASK) {
		syslog(LOG_ERR, "bounce buffer out of ISA range (0x%x)",
		       bouncepa);
		exit(1);
	}

	/*
	 * Initialize waiter's queue
	 */
	ll_init(&waiters);

	/*
	 * Ensure that we start with motors turned off - reset the FDC
	 * while we're at it!
	 */
	outportb(fd_baseio + FD_MOTOR, 0);
	motors_off();

	if (fdc_type == FDC_HAVE_UNKNOWN) {
		/*
		 * Find out the controller ID
		 */
		fdc_out(FDC_VERSION);
		c = fdc_in();
		if (c == FDC_VERSION_765A) {
			fdc_type = FDC_HAVE_765A;
		} else if (c == FDC_VERSION_765B) {
			/*
			 * OK, we have at least a 765B, but we may have an
			 * 82077 - we look for some unique features to see
			 * which.  Note that we can only assume bits 0 and
			 * 1 have to respond in the TDR if it's present
			 */
			uchar s1, s2, s3;

			fdc_type = FDC_HAVE_765B;

			s1 = inportb(fd_baseio + FD_TAPE);
			outportb(fd_baseio + FD_TAPE, 0x01);
			s2 = inportb(fd_baseio + FD_TAPE);
			outportb(fd_baseio + FD_TAPE, 0x02);
			s3 = inportb(fd_baseio + FD_TAPE);
			outportb(fd_baseio + FD_TAPE, s1);
			if (((s2 & 0x03) == 0x01) && ((s3 & 0x03) == 0x02)) {
				fdc_type = FDC_HAVE_82077;
			}
		}

		if (fdc_type == FDC_HAVE_UNKNOWN) {
			/*
			 * We've not found an FDC that we recognise - flag a
			 * fail and exit the server
			 */
			syslog(LOG_ERR, "unable to find FDC - aborting!");
			exit(1);
		}
	}

	syslog(LOG_INFO, "FDC %s on IRQ %d, DMA %d, I/O base 0x%x",
		fdc_names[fdc_type], fd_irq, fd_dma, fd_baseio);

	/*
	 * Probe floppy presences
	 */
	outportb(RTCSEL, NV_FDPORT);
	types = inportb(RTCDATA);
	fl = floppies;
	for (x = 0; x < NFD; ++x, ++fl) {
		fl->f_unit = x;
		fl->f_spinning = 0;
		fl->f_opencnt = 0;
		fl->f_mediachg = 0;
		fl->f_chgactive = 0;
		fl->f_state = F_CLOSED;
		fl->f_type = (types >> (4 * (NFD - 1 - x))) & TYMASK;
		fl->f_errors = 0;
		fl->f_retries = FD_MAXERR;
		fl->f_specialdens = DISK_AUTOPROBE;
		fl->f_lastuseddens = DISK_AUTOPROBE;
		fl->f_userp.f_size = FD_PUNDEF;
		fl->f_prbcount = FD_NOPROBE;
		fl->f_messages = FDM_FAIL;
		fl->f_abort = FALSE;

		switch(fl->f_type) {
		case FDNONE:
			fl->f_state = F_NXIO;
			break;

		case FD2880:
			syslog(LOG_INFO, "unit %d: 2.88M\n", x);
			fl->f_posdens = densities[FD2880 - 1];
			break;

		case FD1440:
			syslog(LOG_INFO, "unit %d: 1.44M\n", x);
			fl->f_posdens = densities[FD1440 - 1];
			break;

		case FD720:
			syslog(LOG_INFO, "unit %d: 720k\n", x);
			fl->f_posdens = densities[FD720 - 1];
			break;

		case FD1200:
			syslog(LOG_INFO, "unit %d: 1.2M", x);
			fl->f_posdens = densities[FD1200 - 1];
			break;

		case FD360:
			syslog(LOG_INFO, "unit %d: 360k", x);
			fl->f_posdens = densities[FD360 - 1];
			break;

		default:
			syslog(LOG_INFO, "unit %d: unknown type", x);
			fl->f_state = F_NXIO;
		}
		configed += (fl->f_state != F_NXIO ? 1 : 0);

		if (fl->f_state != F_NXIO) {
			/*
			 * Allocate read-ahead buffer space
			 */
			fl->f_rabuf = (void *)malloc(RABUFSIZE);
			fl->f_rablock = -1;
			fl->f_racount = 0;
			fl->f_ranow = 0;
		}
	}
	if (!configed) {
		syslog(LOG_INFO, "no drives found - server exiting!");
		exit(1);
	}
}


/*
 * fd_isr()
 *	We have received a floppy interrupt.  Drive the state machine.
 */
void
fd_isr(void)
{
	/*
	 * If we're not yet configured then this is just spurious and we
	 * don't want to know!
	 */
	if (configed) {
		state_machine(FEV_INTR);
	}
}


/*
 * abort_io()
 *	Cancel floppy operation
 */
void
abort_io(struct file *f)
{
	ASSERT(busy, "fd abort_io: not busy");

	if (f == cur_tran()) {
		/*
		 * We have to be pretty heavy-handed.  We reset the
		 * controller to stop DMA and seeks, and leave the
		 * floppy in a state where it'll be completely reset by
		 * the next user.  We also clear out any wired DMA pages.
		 */
		timeout(0);
		if (map_handle >= 0) {
			(void)page_release(map_handle);
		}
		busy->f_abort = TRUE;
		busy->f_state = F_RESET;
		unit_reset(F_READY, F_RESET);
	}

	ll_delete(f->f_list);
	f->f_list = 0;
	if (f->f_local) {
		free(f->f_buf);
	}
}


/*
 * fd_time()
 *	A time event has happened
 */
void
fd_time(void)
{
	state_machine(FEV_TIME);
}


/*
 * unit_reset()
 *	Send reset command
 */
static void
unit_reset(int old, int new)
{
	if (old == F_IO) {
		/*
		 * We're reseting after a failed I/O operation
		 */
		busy->f_errors++;
	}

	/*
	 * We handle resets differently, depending on the controller types
	 */
	if (fdc_type == FDC_HAVE_82077) {
		outportb(fd_baseio + FD_DRSEL, F_SWRESET);
	} else {
		outportb(fd_baseio + FD_MOTOR, motor_mask());
		__usleep(RESET_SLEEP);
		motors();
	}

	timeout(2000);
}


/*
 * unit_failed()
 *	Unit won't even reset--mark as non-existent
 */
static void
unit_failed(int old, int new)
{
	if (!busy)
		return;
	syslog(LOG_ERR, "unit %d: won't reset - deconfiguring", busy->f_unit);
	failed(ENXIO);
	busy->f_state = F_NXIO;
}


/*
 * fd_close()
 *	Process a close on a device
 */
void
fd_close(struct msg *m, struct file *f)
{
	struct floppy *fl;
	
	/*
	 * If it's just at the directory level, no problem
	 */
	if (f->f_slot == ROOTDIR) {
		return;
	}
	
	/*
	 * Abort any active I/O
	 */
	if (f->f_list) {
		abort_io(f);
	}
	
	/*
	 * Decrement open count
	 */
	fl = unit(f->f_unit);
	fl->f_opencnt -= 1;
}


/*
 * media_chgtest()
 *	Check the status of the diskette change-line
 *
 * Look at the diskette change-line status and decide if this is new media
 * in the drive.  If it is, mark in the per-floppy structure that a change
 * is active (a seek will clear it) and increment the count of media
 * changes.  If we were active and are now clear, mark the floppy state
 * as being no-change-active
 */
static void
media_chgtest(void)
{
	if (inportb(fd_baseio + FD_DIGIN) & FD_MEDIACHG) {
		if (!busy->f_chgactive) {
			busy->f_chgactive = 1;
			busy->f_mediachg++;
			busy->f_rablock = -1;
			busy->f_racount = 0;
			if (cur_tran()->f_slot == SPECIALNODE) {
				media_probe(cur_tran());
			}
			if (busy->f_messages <= FDM_WARNING) {
				syslog(LOG_INFO, "unit %d: media changed",
					busy->f_unit);
			}
		}
	} else {
		if (busy->f_chgactive) {
			busy->f_chgactive = 0;
		}
	}
}


/*
 * media_probe()
 *	Initiate a check on the diskette media
 *
 * If we're into autoprobing the media, then we kick of an autoprobe hunt
 * sequence.  The remainder of the sequence will be handled in unit_iodone()
 */
static void
media_probe(struct file *f)
{
	/*
	 * Don't try and start a probe if we're already doing one
	 */
	if (busy->f_prbcount != FD_NOPROBE) {
		return;
	}

	/*
	 * If we're running user defined parameters then this was
	 * a bad request - flag it as such
	 */
	if (busy->f_density == DISK_USERDEF) {
		failed(EIO);
		return;
	}

	/*
	 * Ah, then our only excuse is that we should be autoprobing
	 * the media type.  We need to create a pseudo file that
	 * can then be ammended to give the impression of a client
	 * reading the last sector of the disk - we keep trying
	 * until we find a sector or run out of possibilities.  We
	 * do our I/O into the bounce buffer - nothing else can use
	 * it while we're running.
	 */
	ASSERT_DEBUG(busy->f_density == DISK_AUTOPROBE,
		     "fd fd_rw: bad media parameters");

	busy->f_prbcount = 0;
	busy->f_parms = fdparms[busy->f_posdens[busy->f_prbcount]];
	busy->f_lastuseddens = DISK_AUTOPROBE;
	prbf = *f;
	prbf.f_local = 0;
	prbf.f_count = SECSZ(busy->f_parms.f_secsize);
	prbf.f_buf = bounceva;
	prbf.f_unit = f->f_unit;
	prbf.f_slot = 0;
	prbf.f_blkno = (busy->f_parms.f_size
			/ SECSZ(busy->f_parms.f_secsize)) - 1;
	calc_cyl(&prbf, &busy->f_parms);
	prbf.f_dir = FS_READ;
	prbf.f_off = 0;

	state_machine(FEV_WORK);
}
@


1.10
log
@Merge in massive fixes for the floppy driver.  It now pretty
much works!
@
text
@d43 1
d554 15
d819 5
a823 1
	fdc_out((f->f_blkno % fp->f_sectors) + 1);
d843 12
d871 1
a871 1
	if ((((uint)(f->f_buf) + f->f_off) & (NBPG - 1)) + secsz <= NBPG) {
d876 1
a876 1
		map_handle = page_wire(f->f_buf + f->f_off, (void **)&pa);
d901 1
a901 1
			bcopy(f->f_buf + f->f_off, bounceva, secsz);
d917 50
d1007 44
a1069 1
	struct msg m;
d1185 5
d1197 7
a1203 1
		bcopy(bounceva, f->f_buf + f->f_off, secsz);
a1206 5
	 * Clear error count after successful transfer
	 */
	busy->f_errors = 0;

	/*
d1211 15
a1225 9
	f->f_pos += secsz;
	f->f_off += secsz;
	f->f_blkno += 1;
	f->f_count -= secsz;
	if (f->f_count > 0) {
		calc_cyl(f, &busy->f_parms);
		busy->f_state = F_SEEK;
		unit_seek(F_READY, F_SEEK);
		return;
d1228 10
a1237 29
	/*
	 * All done. Return results.  If we used a local buffer, send it
	 * back.  Otherwise we DMA'ed into his own memory, so no segment
	 * is returned.
	 */
	if (f->f_local) {
		m.m_buf = f->f_buf;
		m.m_buflen = f->f_off;
		m.m_nseg = 1;
	} else {
		m.m_nseg = 0;
	}
	m.m_arg = f->f_off;
	m.m_arg1 = 0;
	msg_reply(f->f_sender, &m);

	/*
	 * Dequeue the completed operation.  Kick the queue so
	 * that any pending operation can now take over.
	 */
	ll_delete(f->f_list);
	f->f_list = 0;
	run_queue();

	/*
	 * He has it, so free back to our pool
	 */
	if (f->f_local) {
		free(f->f_buf);
d1240 1
a1240 7
	/*
	 * Leave a timer behind; if nothing comes up, this will
	 * cause the floppies to spin down.
	 */
	if (!busy) {
		timeout(3000);
	}
d1518 1
d1555 10
a1598 7
	ll_delete(f->f_list);
	f->f_list = 0;
	if (f->f_local) {
		free(f->f_buf);
	}
	msg_err(f->f_sender, EIO);

d1610 9
a1618 5
		outportb(fd_baseio + FD_MOTOR, 0);
		__usleep(RESET_SLEEP);
		motors_off();
		busy->f_state = F_CLOSED;
		run_queue();
d1686 1
a1686 1

d1693 1
a1693 1

d1700 1
a1700 1

d1702 1
a1702 2
	 * Decrement open count.  On last close, shut off motor to make
	 * it easier to swap floppies.
d1705 1
a1705 6
	if ((fl->f_opencnt -= 1) > 0)
		return;
	ASSERT_DEBUG(busy != fl, "fd fd_close: closed but busy");
	fl->f_state = F_CLOSED;
	fl->f_spinning = 0;
	motors();
d1726 2
@


1.9
log
@Convert to openlog()
@
text
@d9 2
a10 5
 * time and do true raw I/O.  If the memory crosses a 64K boundary
 * or isn't aligned, we use a bounce buffer.
 *
 * This driver does not support 360K drives, nor 360K floppies in
 * a 1.2M drive.
d13 1
d19 1
a19 1
#include <std.h>
d21 1
d24 35
a58 9
static void unit_spinup(), unit_recal(), unit_seek(), unit_spindown(),
	unit_io(), unit_iodone(), failed(), state_machine(),
	unit_reset(), unit_failed(), unit_settle();

extern port_t fdport;			/* Our server port */
struct floppy floppies[NFD];		/* Per-floppy state */
static void *bounceva, *bouncepa;	/* Bounce buffer */
static int errors = 0;			/* Global error count */
static int map_handle;			/* Handle for DMA mem mapping */
d60 1
d90 1
a90 1
	{F_SEEK,	FEV_INTR,	unit_settle,	F_SETTLE},
a91 1
	{F_SETTLE,	FEV_TIME,	unit_io,	F_IO},
d94 1
a94 1
	{F_READY,	FEV_WORK,	unit_seek,	F_SEEK},
d103 9
d113 4
a116 1
 * Different formats of floppies
d119 56
a174 2
	{15, 0x1B, 80, 2400},	/* 1.2 meg */
 	{18, 0x1B, 80, 2880}	/* 1.44 meg */
d177 1
d184 1
d192 1
a192 1
cur_tran()
d194 4
d204 1
d212 2
a213 1
	int i, j;
d215 9
a223 3
	for (i = 50000; i > 0; --i) {
		j = (inportb(FD_STATUS) & (F_MASTER|F_DIR));
		if (j == (F_MASTER|F_DIR)) {
d227 3
a229 1
			syslog(LOG_ERR, "fdc_in failed");
d232 9
a240 3
	}
	if (i < 1) {
		syslog(LOG_ERR, "fdc_in failed2");
d243 1
a243 1
	return(inportb(FD_DATA));
d246 1
d254 2
a255 1
	int i, j;
d257 7
a263 2
	for (i = 50000; i > 0; --i) {
		j = (inportb(FD_STATUS) & (F_MASTER|F_DIR));
d267 9
a275 3
	}
	if (i < 1) {
		syslog(LOG_ERR, "fdc_out failed");
d278 1
a278 1
	outportb(FD_DATA, c);
d282 1
d285 1
a285 1
 *	Given unit #, return pointer to data structure
d294 1
d297 1
a297 1
 *	Given current block #, generate cyl/head values
d304 2
a305 2
	f->f_cyl = f->f_blkno / (2*fp->f_sectors);
	head = f->f_blkno % (2*fp->f_sectors);
d310 1
a317 2
	struct fdparms *fp = &fdparms[fl->f_density];

d335 1
d337 2
a338 3
	f->f_unit = fl->f_unit;
	f->f_blkno = f->f_pos/SECSZ;
	calc_cyl(f, fp);
d351 1
a362 1
	extern port_name fdname;
d368 1
a368 1
		selfport = msg_connect(fdname, ACC_WRITE);
d379 1
a379 1
	t.t_usec += (child_msecs*1000);
d395 1
d398 1
a398 1
 *	Ask for M_TIME message in requested # of milliseconds
d431 1
d447 1
a447 1
		if (floppies[x].f_spinning)
d449 1
d454 1
d472 1
a472 1
	outportb(FD_MOTOR, FD_INTR);
d475 1
d486 1
a486 1
	if (busy)
d488 2
a489 1
	outportb(FD_MOTOR, motmask);
d492 1
d506 1
a506 1
	timeout(MOT_TIME);
d509 1
d519 1
a519 4
	busy->f_spinning = 0;
	busy->f_state = F_CLOSED;
	motors();
	failed();
d522 1
d526 3
d533 1
a533 1
	uint x;
d538 17
a554 1
	 * Sense status
d556 10
a565 3
	fdc_out(FDC_SENSE);
	fdc_out(0 | busy->f_unit);	/* Always head 0 after reset */
	x = fdc_in();
d568 2
a569 1
	 * Specify some parameters now that reset
d572 2
a573 2
	fdc_out(FD_SPEC1);
	fdc_out(FD_SPEC2);
d575 26
a600 1
	outportb(FD_CTL, XFER_500K);
d610 1
d619 2
a620 1
	uint x;
d626 16
a641 1
	 * Sense result
d643 1
a643 3
	fdc_out(FDC_SENSE);
	fdc_out((f->f_head << 2) | busy->f_unit);
	x = fdc_in();
d646 1
a646 3
	 * If the floppy consistently errors, we will keep re-entering
	 * this state.  It thus makes a good place to put a cap on the
	 * number of errors.
d648 15
a662 7
	if (errors > FD_MAXERR) {
		errors = 0;
		busy->f_spinning = 0;
		busy->f_state = F_CLOSED;
		motors();
		failed();
		return;
d666 2
a667 1
	 * If we're already there, advance to I/O immediately
d670 27
a696 4
	if (busy->f_cyl == f->f_cyl) {
		busy->f_state = F_IO;
		unit_io(new, F_IO);
		return;
d704 2
a705 2
	fdc_out(f->f_cyl);
	busy->f_cyl = f->f_cyl;
d713 1
d721 1
a721 1
run_queue()
d725 2
a726 2
	busy = 0;
	if ((f = cur_tran()) == 0)
d728 1
d733 1
d737 3
d742 1
a742 1
failed(void)
d744 19
a762 1
	struct file *f = cur_tran();
d770 12
a781 1
	msg_err(f->f_sender, EIO);
d789 1
d797 1
a797 1
	struct fdparms *fp = &fdparms[fl->f_density];
d803 2
a804 2
	fdc_out((f->f_blkno % fp->f_sectors)+1);
	fdc_out(2);	/* Sector size--always 2*datalen */
d807 1
a807 1
	fdc_out(0xFF);	/* Data length--256 bytes */
d810 1
a816 3
 *
 * TBD: can we do more than one sector at a time?  I'll play with this
 * in my copious free time....
d822 10
d834 4
a837 2
	 * Try for straight map-down of memory.  Use bounce buffer
	 * if we can't get the memory directly.
d839 19
a857 1
	map_handle = page_wire(f->f_buf + f->f_off, (void **)&pa);
d859 3
d863 8
d872 1
d875 5
a879 5
	outportb(DMA_ADDR, pa & 0xFF);
	outportb(DMA_ADDR, (pa >> 8) & 0xFF);
	outportb(DMA_HIADDR, (pa >> 16) & 0xFF);
	outportb(DMA_CNT, (SECSZ-1) & 0xFF);
	outportb(DMA_CNT, ((SECSZ-1) >> 8) & 0xFF);
d883 1
d899 2
a900 2
	if (old == F_SETTLE) {
		uint x;
d902 20
a921 3
		fdc_out(FDC_SENSE);
		fdc_out((f->f_head << 2) | busy->f_unit);
		x = fdc_in();
d925 1
a925 2
	 * Dequeue an operation.  If the DMA can't be set up,
	 * error the operation and return.
d929 1
a929 1
	timeout(2000);
d932 1
d941 1
d959 7
a965 5
	} else {
		/*
		 * Need to bounce out to buffer
		 */
		bcopy(bounceva, f->f_buf+f->f_off, SECSZ);
d978 51
a1028 2
	if (results[0] & 0xF8) {
		errors += 1;
d1030 26
a1055 1
		unit_recal(F_SPINUP1, F_RECAL);
d1060 11
d1073 1
a1073 1
	errors = 0;
d1080 2
a1081 2
	f->f_pos += SECSZ;
	f->f_off += SECSZ;
d1083 1
a1083 1
	f->f_count -= SECSZ;
d1085 3
a1087 3
		calc_cyl(f, &fdparms[busy->f_density]);
		busy->f_state = F_IO;
		unit_io(F_READY, F_IO);
d1092 2
a1093 10
	 * All done.  Dequeue operation, generate a completion.  Kick
	 * the queue so any pending operation can now take over.
	 */
	ll_delete(f->f_list);
	f->f_list = 0;
	run_queue();

	/*
	 * Return results.  If we used a local buffer, send it back.
	 * Otherwise we DMA'ed into his own memory, so no segment
d1108 8
d1131 1
d1150 4
a1153 2
		printf("  dens = %d, unit = %d, spin = %d, f_cyl = %d, open = %d\n",
		       busy->f_density, busy->f_unit, busy->f_spinning, busy->f_cyl, busy->f_opencnt);
d1197 1
d1203 1
a1203 1
 * of sector sizes, or we EINVAL'em out of here.
d1208 1
d1215 1
a1215 1
		if (f->f_unit == ROOTDIR) {
d1220 4
a1223 2
		/* FS_WRITE */
		if ((f->f_unit == ROOTDIR) || (m->m_nseg != 1)) {
d1230 1
a1230 1
	 * Check size
d1232 1
a1232 3
	if ((m->m_arg & (SECSZ-1)) ||
			(m->m_arg > MAXIO) ||
			(f->f_pos & (SECSZ-1))) {
d1236 1
d1238 26
d1275 1
a1275 1
	 * Unit present?
d1277 11
a1287 3
	if (fl->f_state == F_NXIO) {
		msg_err(m->m_sender, ENXIO);
		return;
d1291 3
a1293 1
	 * Queue I/O to unit
d1301 1
d1304 4
a1307 1
 *	Set format for each NVRAM-configured floppy
d1312 1
a1312 1
	int x;
d1317 14
d1336 2
a1337 1
	 * Ensure that we start with motors turned off
d1339 32
a1370 1
	outportb(FD_MOTOR, 0);
d1372 13
d1391 1
a1391 1
	for (x = 0; x < NFD; ++x, ++fl, types <<= 4) {
d1395 44
a1438 1
		if ((types & TYMASK) == FDNONE) {
a1439 1
			continue;
d1441 1
a1441 14
		if ((types & TYMASK) == FD12) {
			syslog(LOG_INFO, "fd%d: 1.2M", x);
			fl->f_state = F_CLOSED;
			fl->f_density = 0;
			continue;
		}
		if ((types & TYMASK) == FD144) {
			syslog(LOG_INFO, "fd%d: 1.44M\n", x);
			fl->f_state = F_CLOSED;
			fl->f_density = 1;
			continue;
		}
		syslog(LOG_INFO, "fd%d: unknown type", x);
		fl->f_state = F_NXIO;
d1443 2
a1444 7

	/*
	 * Get a bounce buffer for misaligned I/O
	 */
	bounceva = malloc(NBPG);
	if (page_wire(bounceva, &bouncepa) < 0) {
		syslog(LOG_ERR, "can't get bounce buffer");
d1449 1
d1457 7
a1463 1
	state_machine(FEV_INTR);
d1466 1
d1475 9
a1483 8
	if (f != cur_tran()) {
		/*
		 * If it's not the current operation, this is easy
		 */
		ll_delete(f->f_list);
		f->f_list = 0;
		msg_err(f->f_sender, EIO);
	} else {
d1485 2
a1486 2
		 * Otherwise we have to be pretty heavy-handed.  We reset
		 * the controller to stop DMA and seeks, and leave the
d1488 1
a1488 1
		 * the next user.
d1491 5
a1495 4
		ll_delete(f->f_list);
		f->f_list = 0;
		msg_err(f->f_sender, EIO);
		outportb(FD_MOTOR, 0);
d1502 1
d1513 1
d1525 12
a1536 1
		errors++;
a1538 2
	outportb(FD_MOTOR, motor_mask());
	outportb(FD_MOTOR, motor_mask()|FD_INTR);
a1541 9
/*
 * unit_settle()
 *	Pause to let the heads to settle
 */
static void
unit_settle(int old, int new)
{
	timeout(HEAD_SETTLE);
}
d1552 2
a1553 1
	syslog(LOG_ERR, "fd%d: won't reset--deconfiguring", busy->f_unit);
a1554 3
	busy->f_spinning = 0;
	motors();
	failed();
d1557 1
d1570 1
a1570 1
	if (f->f_unit == ROOTDIR) {
d1592 90
@


1.8
log
@Syslog support
@
text
@a29 1
extern char fd_sysmsg[];		/* Syslog message prefix */
d122 1
a122 1
			syslog(LOG_ERR, "%s fdc_in failed", fd_sysmsg);
d127 1
a127 1
		syslog(LOG_ERR, "%s fdc_in failed2", fd_sysmsg);
d149 1
a149 1
		syslog(LOG_ERR, "%s fdc_out failed", fd_sysmsg);
d883 1
a883 1
			syslog(LOG_INFO, "%s fd%d: 1.2M", fd_sysmsg, x);
d889 1
a889 1
			syslog(LOG_INFO, "%s fd%d: 1.44M\n", fd_sysmsg, x);
d894 1
a894 1
		syslog(LOG_INFO, "%s fd%d: unknown type", fd_sysmsg, x);
d903 1
a903 1
		syslog(LOG_ERR, "%s can't get bounce buffer", fd_sysmsg);
d999 1
a999 2
	syslog(LOG_ERR, "%s fd%d: won't reset--deconfiguring",
		fd_sysmsg, busy->f_unit);
@


1.7
log
@Cleanup and switch to syslog
@
text
@d30 1
d123 1
a123 1
			syslog(LOG_ERR, "fd: fdc_in failed\n");
d128 1
a128 1
		syslog(LOG_ERR, "fd: fdc_in failed2\n");
d150 1
a150 1
		syslog(LOG_ERR, "fd: fdc_out failed\n");
d884 1
a884 1
			syslog(LOG_INFO, "fd%d: 1.2M\n", x);
d890 1
a890 1
			syslog(LOG_INFO, "fd%d: 1.44M\n", x);
d895 1
a895 1
		syslog(LOG_INFO, "fd%d: unknown type\n", x);
d904 1
a904 1
		syslog(LOG_ERR, "Can't get bounce buffer\n");
d1000 2
a1001 1
	syslog(LOG_ERR, "fd%d: won't reset--deconfiguring\n", busy->f_unit);
@


1.6
log
@Source reorg
@
text
@d15 1
d19 2
d22 1
d111 1
a111 1
static
d122 1
a122 1
			printf("fdc_in failed\n");
d127 1
a127 1
		printf("fdc_in failed2\n");
d137 1
a137 1
static
d149 1
a149 1
		printf("fdc_out failed\n");
d163 1
a163 1
	ASSERT_DEBUG((u >= 0) && (u < NFD), "fd: bad unit");
d186 1
a186 1
static
d201 1
a201 1
			return;
d307 1
a307 1
static
d513 1
a513 2
	ASSERT_DEBUG(f, "fd failed(): not busy");
	msg_err(f->f_sender, EIO);
d516 1
d563 1
a563 1
	map_handle = page_wire(f->f_buf + f->f_off, &pa);
d737 2
d864 5
d883 1
a883 1
			printf("fd%d 1.2M\n", x);
d889 1
a889 1
			printf("fd%d 1.44M\n", x);
d894 1
a894 1
		printf("fd%d unknown type\n", x);
d903 1
a903 1
		printf("Can't get bounce buffer\n");
d968 7
d999 1
a999 1
	printf("fd%d won't reset--deconfiguring\n", busy->f_unit);
d1036 1
a1036 1
	ASSERT_DEBUG(busy != fl, "fd_close: closed but busy");
@


1.5
log
@Have to pass back *count*; f_count is zero at the end of the
transfer.
@
text
@a15 2
#include <lib/llist.h>
#include <fd/fd.h>
d19 1
@


1.4
log
@Only free buf if local.  Rare case, anyway.
@
text
@d696 1
a696 1
	m.m_arg = f->f_count;
@


1.3
log
@Get read/write up and running!
@
text
@d212 3
a214 1
		free(f->f_buf);
@


1.2
log
@First pass at bringing up floppy drive
@
text
@d24 1
a24 1
	unit_reset(), unit_failed();
d61 1
a61 1
	{F_SEEK,	FEV_INTR,	unit_io,	F_IO},
d63 1
d98 1
a98 1
	if (&waiters == waiters.l_forw)
d100 1
d115 1
a115 1
		if (j == (F_MASTER|F_DIR))
d117 3
a119 1
		if (j == F_MASTER)
d121 1
d123 2
a124 1
	if (i < 1)
d126 1
d141 1
a141 1
		if (j == F_MASTER)
d143 1
d145 2
a146 1
	if (i < 1)
d148 1
d190 16
a209 3
	f->f_nseg = m->m_nseg;
	f->f_buf = m->m_buf;
	f->f_count = m->m_buflen;
d212 1
d223 1
a223 1
static int child_secs;
d247 5
a251 1
	t.t_sec += child_secs;
d265 1
a265 1
 *	Ask for M_TIME message in requested # of seconds
d268 1
a268 1
timeout(int secs)
d277 1
d279 1
a279 1
	if (secs == 0) {
d282 1
a282 1
	child_secs = secs;
d392 2
d395 14
d410 5
d417 1
a417 1
	timeout(4);
d427 2
a428 1
	struct file *f;
d430 1
a430 1
	ASSERT_DEBUG(cur_tran(), "fd seek: no work");
d434 7
d475 1
a475 1
	timeout(5);
a552 1
	int dir;
d555 2
a556 2
	 * Try for straight map-down of user's memory.  Ignore virtual
	 * address of mapping; we're doing DMA in this driver.
a559 3
		/*
		 * Nope.  Use bounce buffer
		 */
d562 2
a563 3
	dir = (f->f_dir == FS_READ) ? DMA_READ : DMA_WRITE;
	outportb(DMA_STAT1, dir);
	outportb(DMA_STAT0, dir);
d567 2
a568 2
	outportb(DMA_CNT, SECSZ & 0xFF);
	outportb(DMA_CNT, (SECSZ >> 8) & 0xFF);
d581 14
a598 2
	ASSERT_DEBUG(f, "fd io: not busy");
	ASSERT_DEBUG(busy, "fd io: not busy");
d601 1
a601 1
	timeout(2);
d624 11
a634 4
	/*
	 * Release memory lock-down, if any
	 */
	(void)page_release(map_handle);
d681 13
a693 1
	m.m_buflen = m.m_nseg = m.m_arg1 = 0;
d695 1
d699 7
d710 1
a710 1
		timeout(3);
d728 9
d762 1
a762 1
	if (!s->s_state)
d764 1
d770 1
a770 1
	if (s->s_fun)
d772 1
d795 6
a800 3
	} else if (f->f_unit == ROOTDIR) {
		msg_err(m->m_sender, EINVAL);
		return;
d807 1
d871 1
a871 1
			printf("Drive %d 1.2M\n", x);
d877 1
a877 1
			printf("Drive %d 1.44M\n", x);
d882 1
a882 1
		printf("Unknown floppy type, drive %d\n", x);
d958 11
a968 1
	timeout(2);
@


1.1
log
@Initial revision
@
text
@d20 1
a28 1
static seg_t bouncehandle;
d30 1
a182 2
	f->f_count = m->m_arg;
	f->f_off = 0;
d185 3
a187 1
	bcopy(m->m_seg, f->f_seg, f->f_nseg*sizeof(seg_t));
d196 40
d242 12
a253 1
	struct time t;
d255 6
a260 7
	if (secs) {
		(void)time(&t.t_sec);
		t.t_sec += secs;
		t.t_usec = 0L;
		alarm_set(fdport, &t);
	} else {
		alarm_set(fdport, 0);
d262 5
d498 2
a499 1
	 * Try for straight map-down of user's memory
d501 2
a502 1
	if (seg_physmap(f->f_seg, f->f_nseg, f->f_off, SECSZ, &pa)) {
d560 5
d782 2
a783 1
	if (seg_getpage(&bounceva, &bouncepa, 1, &bouncehandle) < 0) {
@
