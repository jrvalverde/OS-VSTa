head	1.7;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.7
date	94.11.03.01.34.47;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.10.05.23.26.56;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.04.10.19.47.52;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.45.04;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.10.18.08.16;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.43.50;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.47.14;	author vandys;	state Exp;
branches;
next	;


desc
@Floppy hardware and message definitions
@


1.7
log
@Add optimizations using track readahead
@
text
@#ifndef _FD_H
#define _FD_H
/*
 * fd.h
 *	Declarations for PC/AT floppy disks
 */
#include <sys/types.h>
#include <sys/msg.h>
#include <llist.h>
#include <mach/nvram.h>


/*
 * # floppy units supported
 */
#define NFD 2


/*
 * Max I/O size supported
 */
#define MAXIO 65536


/*
 * Read-ahead buffer size
 */
#define RABUFSIZE 20480


/*
 * Retry levels
 */
#define FD_MAXERR 4		/* Default maximum retries on normal ops */
#define FD_PROBEERR 2		/* Maximum number of retries on probe ops */


/*
 * Description of a type of floppy
 */
struct fdparms {
	int f_size;		/* Size of disk in bytes */
	int f_tracks;		/* Total tracks */
	int f_heads;		/* Number of heads */
	int f_sectors;		/* Sectors/track */
	uchar f_stretch;	/* Additional steps on drive per media track */ 
	uchar f_gap;		/* Gap between sectors */
	uchar f_fmtgap;		/* Gap for formatting track */
	uchar f_rate;		/* Data rate | 0x80 for perpendicular mode*/
	uchar f_spec1;		/* Specify byte 1 - HUT and SRT */
	uchar f_spec2;		/* Specify byte 2 - HLT */
	uchar f_secsize;	/* How big are the sectors in the disk drive? */
};


#define FD_PUNDEF 0		/* f_size == FD_PUNDEF when fdparm's invalid */

#define SECTOR_128 0		/* 128 bytes per sector */
#define SECTOR_256 1		/* 256 bytes per sector */
#define SECTOR_512 2		/* 512 bytes per sector */
#define SECTOR_1024 3		/* 1024 bytes per sector */
#define SECTOR_2048 4		/* 2048 ... */
#define SECTOR_4096 5
#define SECTOR_8192 6
#define SECTOR_16384 7

#define SECSZ(s) (128 << s)	/* Size of a sector in bytes */

extern struct fdparms fdparms[];


/*
 * Per-floppy state
 */
struct floppy {
	struct llist f_q;	/* List of pending work */
	struct fdparms f_parms;	/* Our current floppy parameter table */
	struct fdparms f_userp;	/* User defined floppy parameters */
	uchar f_unit;		/* Our unit number */
	uchar f_state;		/* State (see below) */
	char f_density;		/* What format floppy do we have open? */
	char f_specialdens;	/* What format for the special node? */
	char f_lastuseddens;	/* What was the last used density? */
	uchar f_spinning;	/* Our motor is on */
	uchar f_type;		/* What type of drive is this? */
	uint f_cyl;		/* Cylinder heads are currently on */
	uint f_opencnt;		/* Number of open references */
	int *f_posdens;		/* Array of possible densities */
	uchar f_abort;		/* Are we in an abort cycle */
	char f_prbcount;	/* Which density are we probing (if any)? */
	int f_mediachg;		/* Number of media changes */
	uchar f_chgactive;	/* Is the change-line active? */
	int f_errors;		/* Number of consecutive errors */
	int f_retries;		/* Number of retries permitted */
	uchar f_messages;	/* Message issuing level */	
	void *f_rabuf;		/* Read ahead buffer */
	int f_rablock;		/* First block in the r/a buffer */
	int f_racount;		/* Number of blocks in the r/a buffer */
	int f_ranow;		/* Are we doing a read-ahead at the moment */
};

#define FD_NOPROBE -1		/* We're not probing media details */

extern struct floppy floppies[NFD];


/*
 * Media density values
 */
#define DISK_DENSITIES 10	/* Number of diskette media parameters */
#define DISK_AUTOPROBE -1	/* Autoprobe the density */
#define DISK_360_360 0		/* 360k in a 360k drive */
#define DISK_360_720 1		/* 360k in a 720k drive */
#define DISK_360_1200 2		/* 360k in a 1.2M drive */
#define DISK_720_720 3		/* 720k in a 720k drive */
#define DISK_720_1200 4		/* ... are you seeing a pattern here? */
#define DISK_1200_1200 5
#define DISK_1200_1440 6
#define DISK_1440_1440 7
#define DISK_1440_2880 8
#define DISK_2880_2880 9
#define DISK_USERDEF 10		/* User defined media parameters */


/*
 * f_state values
 */
#define F_OFF 1			/* Motor off, but device open */
#define F_CLOSED 2		/* No user */
#define F_SPINUP1 3		/* Motor starting for recal */
#define F_SPINUP2 4		/* Motor starting for same floppy */
#define F_RECAL 5		/* Recalibrating */
#define F_READY 6		/* Format known, ready for work */
#define F_SEEK 7		/* Head in motion */
#define F_IO 8			/* Head at track, I/O in progress */
#define F_NXIO 9		/* Unit does not exist */
#define F_RESET 10		/* Reset in progress */


/*
 * Event values for state machine
 */
#define FEV_TIME 1		/* Timer has popped */
#define FEV_INTR 2		/* Received interrupt */
#define FEV_WORK 3		/* More work has arrived */
#define FEV_CLOSED 4		/* All done - event to allow a floppy switch */


/*
 * State transition table
 */
struct state {
	int s_state;		/* Current state */
	int s_event;		/* Event */
	voidfun s_fun;		/* Action */
	int s_next;		/* New state */
};


/*
 * I/O ports offsets
 */
#define FD_MOTOR 0x2
#define FD_TAPE 0x3		/* Tape drive reg - only available on 82077 */
#define FD_STATUS 0x4
#define FD_DRSEL 0x4
#define FD_DATA 0x5
#define FD_CTL 0x7
#define FD_DIGIN 0x7
#define FD_LOW FD_MOTOR
#define FD_HIGH FD_CTL


/*
 * FDC commands
 */
#define FDC_RECAL 0x07
#define FDC_SEEK 0x0f
#define FDC_READ 0xe6
#define FDC_WRITE 0xc5
#define FDC_SENSEI 0x08
#define FDC_SENSE 0x04
#define FDC_SPECIFY 0x03
#define FDC_VERSION 0x10
#define FDC_CONFIGURE 0x13
#define FDC_PERPENDICULAR 0x12


/*
 * Values for transfer rate (FD_CTL)
 */
#define XFER_500K 0
#define XFER_300K 1
#define XFER_250K 2
#define XFER_1M 3


/*
 * Values for perpendicular transfer rate
 */
#define PXFER_TSFR 0x80
#define PXFER_MASK 0x7f
#define PXFER_1M 3
#define PXFER_500K 2


/*
 * Bits for FD_MOTOR
 */
#define FD_MOTMASK 0x10		/* Starting here, bits for motors */
#define FD_INTR 0x0C		/* Enable interrupts */
#define FD_UNITMASK 0x03	/* Mask for unit numbers */


/*
 * Bits for FD_STATUS
 */
#define F_MASTER 0x80		/* Whether he's listening */
#define F_DIR 0x40		/* Direction of data */
#define F_CMDBUSY 0x10		/* There's a command running */


/*
 * Bits for FD_DRSEL
 */
#define F_SWRESET 0x80		/* Trigger s/w reset on 82077 */


/*
 * Default interrupt, DMA vector and base I/O address 
 */
#define FD_IRQ 6
#define FD_DRQ 2
#define FD_BASEIO 0x3f0


/*
 * Argument to FDC_CONFIGURE */
#define FD_CONF1 0x00		/* Zero parameter */
#define FD_CONF2_NOSTRETCH 0x5a	/* Enable implied seeks, enable FIFO */
#define FD_CONF2_STRETCH 0x1a	/* Disable implied seeks, enable FIFO */
#define FD_CONF3 0x00		/* Pre-compensation start track */


/*
 * Controller version IDs
 */
#define FDC_VERSION_765A 0x80	/* Standard type of FDC */
#define FDC_VERSION_765B 0x90	/* Enhanced FDC */


/*
 * Bit to sense diskette change
 */
#define FD_MEDIACHG 0x80


/*
 * Status reg 1 bits
 */
#define FD_POLLING 0xc0		/* Polling error */


/*
 * Status reg 1 bits
 */
#define FD_WRITEPROT 0x02	/* Media is write protected */
#define FD_CRC 0x20		/* CRC data error */


/*
 * Status reg 2 bits
 */
#define FD_CRC 0x20		/* CRC data error */
#define FD_WRONGCYL 0x10	/* We're not on track! */


/*
 * Some time intervals
 */
#define MOTOR_TIME 500		/* Motor spinup time (ms) */
#define IO_TIMEOUT 250000	/* Command I/O timeout (us) */
#define RESET_SLEEP 20		/* Pause to allow FDC reset to happen (us) */


/*
 * Criticality of problem to warrant syslog messages
 */
#define FDM_ALL 0		/* All messages are issued */
#define FDM_WARNING 1		/* Warning and more urgent messages issued */
#define FDM_FAIL 2		/* Failure and more urgent messages issued */
#define FDM_CRITICAL 3		/* Only critical failures are noted */


/*
 * Value for f_node
 */
#define ROOTDIR (0xff)		/* Root */ 
#define SPECIALNODE (0x1f)	/* Special node - can do clever things */
#define UNITSTEP (0x20)		/* Node number step between units */
#define NODE_UNIT(n) ((n >> 5) & 0xff)
				/* Unit number */
#define NODE_SLOT(n) (n & 0x1f)	/* Slot of unit */
#define MKNODE(unit, slot) ((((unit) & 0xff) << 5) | (slot))
				/* Merge unit and slot to make node number */


/*
 * Structure for per-connection operations
 */
struct file {
	int f_sender;		/* Sender of current operation */
	uint f_flags;		/* User access bits */
	struct llist *f_list;	/* When operation pending on this file */
	uchar f_slot;		/* Slot number within current unit */
	uchar f_unit;		/* Current device unit number */
	uint f_blkno;		/* Block number for operation */
	int f_cyl;		/* ... cylinder it's on */
	int f_head;		/* ... head */
	uint f_count;		/* Bytes wanted for current op */
	uint f_off;		/* Offset within current buffer */
	ushort f_dir;		/* FS_READ, FS_WRITE */
	void *f_buf;		/* Base of buffer for operation */
	off_t f_pos;		/* Offset into device */
	int f_local;		/* f_buf is a local buffer */
};


/*
 * Constants for manipulating configuration information on floppy drives.
 */
#define NV_FDPORT 0x10		/* Slot number for FD info */
#define TYMASK 0x0f		/* Mask for density */
#define FDNONE 0x00		/* Drive types */
#define FD360 0x01
#define FD1200 0x02
#define FD720 0x03
#define FD1440 0x04
#define FD2880 0x05
#define FDTYPES FD2880


/*
 * How we keep track of the FDC type
 */
#define FDC_HAVE_UNKNOWN 0x00
#define FDC_HAVE_765A 0x01
#define FDC_HAVE_765B 0x02
#define FDC_HAVE_82077 0x03
#define FDC_NAMEMAX 8


/*
 * Function prototypes - dir.c
 */
void fd_readdir(struct msg *m, struct file *f);
void fd_open(struct msg *m, struct file *f);


/*
 * Function prototypes - rw.c
 */
void fd_close(struct msg *m, struct file *f);
void fd_rw(struct msg *m, struct file *f);
void fd_init(void);
void fd_isr(void);
void abort_io(struct file *f);
void fd_time(void);


/*
 * Function prototypes - stat.c
 */
void fd_stat(struct msg *m, struct file *f);
void fd_wstat(struct msg *m, struct file *f);


#endif /* _FD_H */
@


1.6
log
@Merge in massive fixes for the floppy driver.  It now pretty
much works!
@
text
@d22 1
a22 1
#define MAXIO (65536)
d26 6
d89 1
d96 4
@


1.5
log
@Cleanup and switch to syslog
@
text
@d12 1
d18 1
d24 1
d26 1
a26 1
 * # times to retry an I/O before giving up
d28 2
a29 5
#ifndef DEBUG
#define FD_MAXERR 5
#else
#define FD_MAXERR 2
#endif
d31 1
d33 1
a33 2
 * Special value for f_unit to indicate we're in the root dir
 * of the devices.
d35 25
a59 1
#define ROOTDIR (999)
d61 5
d71 3
d75 3
a77 2
	uchar f_density;	/* What format floppy */
	uchar f_unit;		/* Our unit # */
d79 1
d81 8
a88 1
	uint f_opencnt;		/* # open references */
d90 3
d95 1
d97 1
a97 1
 * Description of a type of floppy
d99 14
a112 7
struct fdparms {
	int f_sectors;		/* sectors/track */
	int f_gap;		/* gap between sectors */
	int f_tracks;		/* total tracks */
	int f_size;		/* size of disk, sectors */
};
extern struct fdparms fdparms[];
d117 20
a136 19
#define F_OFF 1		/* Motor off, but device open */
#define F_CLOSED 2	/* No user */
#define F_SPINUP1 3	/* Motor starting for recal */
#define F_SPINUP2 4	/* Motor starting for same floppy */
#define F_RECAL 5	/* Recalibrating */
#define F_READY 6	/* Format known, ready for work */
#define F_SEEK 7	/* Head in motion */
#define F_IO 8		/* Head at track, I/O in progress */
#define F_NXIO 9	/* Unit does not exist */
#define F_RESET 10	/* Reset in progress */
#define F_SETTLE 11	/* Head settle time after seek */

/*
 * event values for state machine
 */
#define FEV_TIME 1	/* Timer has popped */
#define FEV_INTR 2	/* Received interrupt */
#define FEV_WORK 3	/* More work has arrived */
#define FEV_CLOSED 4	/* All done--an event so we can allow floppy switch */
d148 1
d150 1
a150 2
 * I/O ports used.  Sadly, we straddle two ranges--one for the FDC,
 * and another for the DMAC.
d152 9
a160 7
#include <mach/dma.h>
#define FD_LOW DMA_LOW

#define FD_MOTOR 0x3F2
#define FD_STATUS 0x3F4
#define FD_DATA 0x3F5
#define FD_CTL 0x3F7
a161 1
#define FD_HIGH FD_DATA
d166 4
a169 4
#define FDC_RECAL 0x7
#define FDC_SEEK 0xF
#define FDC_READ 0xE6
#define FDC_WRITE 0xC5
d173 3
d177 1
d184 10
a193 1
#define XFER_125K 3
d195 1
d201 2
a202 1
#define FD_UNITMASK 0x3		/* Mask for unit #'s */
d209 50
d261 1
a261 1
 * Interrupt, DMA vectors
d263 2
a264 2
#define FD_IRQ 6	/* Hardware IRQ6==interrupt vector 14 */
#define FD_DRQ 2	/*  ...DMA channel 2 */
d266 1
d268 1
a268 2
 * Arguments to FDC_SPECIFY command.  They appear to be magic; I got
 * them from my MINIX book.
d270 4
a273 2
#define FD_SPEC1 0xdf
#define FD_SPEC2 0x02
d276 1
a276 1
 * Basic unit of transfer
d278 4
a281 1
#define SECSZ (512)
d283 1
d285 1
a285 1
 * Some time intervals, in milliseconds
d287 9
a295 2
#define HEAD_SETTLE (100)	/* Head settle time after seek */
#define MOT_TIME (500)		/* Motor spinup time */
d301 14
a314 14
	int f_sender;	/* Sender of current operation */
	uint f_flags;	/* User access bits */
	struct llist	/* When operation pending on this file */
		*f_list;
	ushort f_unit;	/* Unit we want */
	uint f_blkno;	/* Block # for operation */
	int f_cyl;	/*  ...cylinder it's on */
	int f_head;	/*  ...head */
	uint f_count;	/* # bytes wanted for current op */
	uint f_off;	/* Offset within current buffer */
	ushort f_dir;	/* FS_READ, FS_WRITE */
	void *f_buf;	/* Base of buffer for operation */
	off_t f_pos;	/* Offset into device */
	int f_local;	/* f_buf is a local buffer */
d317 1
d319 1
a319 2
 * Constants for manipulating configuration information
 * on floppy drives.
d321 20
a340 5
#define NV_FDPORT 0x10		/* Slot # for FD info */
#define TYMASK 0xF0		/* Mask for density */
#define FDNONE 0x00		/* Density: not present, 1.2, 1.44 Mb */
#define FD12 0x20
#define FD144 0x40
d348 1
d358 1
@


1.4
log
@Source reorg
@
text
@d195 23
@


1.3
log
@Get read/write up and running!
@
text
@d9 1
a9 1
#include <lib/llist.h>
@


1.2
log
@Don't use scatter/gather yet
@
text
@d18 5
d25 1
d27 3
d75 1
d105 1
d116 11
d142 1
a142 1
 * How long to wait for motor spinup
d144 2
a145 1
#define MOT_TIME 3
d148 2
a149 1
 * Interrupt, DMA vectors
d151 2
a152 2
#define FD_IRQ 6	/* Hardware IRQ6==interrupt vector 14 */
#define FD_DRQ 2	/*  ...DMA channel 2 */
d160 6
a179 1
	ushort f_nseg;	/* # segments for I/O */
d182 1
@


1.1
log
@Initial revision
@
text
@d151 2
a152 3
	seg_t		/*  ...the segments */
		f_seg[MSGSEGS];
	off_t f_pos;		/* Offset into device */
@
