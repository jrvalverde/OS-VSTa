head	1.9;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.7
	V1_1:1.7
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.9
date	94.10.06.01.56.15;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.03.04.02.02.21;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.45.20;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.10.14.03.40.13;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.09.19.19.14.23;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.02.20.17.58;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.06.30.19.56.58;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.16.14.11.18;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.12.19.57.33;	author vandys;	state Exp;
branches;
next	;


desc
@Central definitions for WD hardware and support data structures
@


1.9
log
@Add support for multiple controllers and such
@
text
@#ifndef _WD_H
#define _WD_H
/*
 * wd.h
 *	Western Digital ST-506 type hard disk interface definitions
 */
#include <sys/types.h>
#include <sys/perm.h>
#include <mach/dpart.h>

#define NWD (2)			/* Max # WD units supported */

/*
 * Undefine this if your WD controller can't switch heads by itself.
 * Most can, and it's much more efficient, so we leave it on by default.
 */
#define AUTO_HEAD

#define SECSZ (512)		/* Only 512 byte sectors handled */
#define MAXIO (128*1024)	/* Max I/O--128K */

#define WD1_PORT 0x1f0		/* I/O ports here */
#define WD2_PORT 0x170		/* Secondary I/O ports start here */
#define WD1_IRQ	14		/* IRQ number for hard disk ctrl */
#define WD2_IRQ	15		/* Usual IRQ number for second HDD ctrl */

/*
 * Registers on WD controller
 */
#define WD_DATA		0	/* Data */
#define WD_ERROR	1	/* Error */
#define WD_SCNT		2	/* Sector count */
#define WD_SNUM		3	/* Sector number */
#define WD_CYL0		4	/* Cylinder, low 8 bits */
#define WD_CYL1		5	/*  ...high 8 bits */
#define WD_SDH		6	/* Sector size, drive and head */
#define WD_STATUS	7	/* Command or immediate status */
#define WD_CMD		7
#define WD_CTLR		0x206	/* Controller port */

/*
 * Status bits
 */
#define WDS_ERROR	0x1	/* Error */
#define WDS_ECC		0x4	/* Soft ECC error */
#define WDS_DRQ		0x8	/* Data request */
#define WDS_BUSY	0x80	/* Busy */

/*
 * Parameters for WD_SDH
 */
#define WDSDH_512	0x20
#define WDSDH_EXT	0x80

/*
 * Bits for controller port
 */
#define CTLR_IDIS 0x2		/* Disable controller interrupts */
#define CTLR_RESET 0x4		/* Reset controller */
#define CTLR_4BIT 0x8		/* Use four head bits */

/*
 * Commands
 */
#define WDC_READ	0x20	/* Command: read */
#define WDC_WRITE	0x30	/*  ...write */
#define WDC_READP	0xEC	/*  ...read parameters */
#define WDC_DIAG	0x90	/* Run controller diags */
#define WDC_SPECIFY	0x91	/* Initialise controller parameters */

/*
 * Read parameters command (WDC_READP) returns this.  I think this
 * struct comes from CMU Mach originally.
 */
struct wdparameters {

	/* drive info */
	ushort	w_config;		/* general configuration */
	ushort	w_fixedcyl;		/* number of non-removable cylinders */
	ushort	w_removcyl;		/* number of removable cylinders */
	ushort	w_heads;		/* number of heads */
	ushort	w_unfbytespertrk;	/* number of unformatted bytes/track */
	ushort	w_unfbytes;		/* number of unformatted bytes/sector */
	ushort	w_sectors;		/* number of sectors */
	ushort	w_minisg;		/* minimum bytes in inter-sector gap*/
	ushort	w_minplo;		/* minimum bytes in postamble */
	ushort	w_vendstat;		/* number of words of vendor status */

	/* controller info */
	char	w_cnsn[20];		/* controller serial number */
	ushort	w_cntype;		/* controller type */
#define	WDTYPE_SINGLEPORTSECTOR	1	 /* single port, single sector buffer */
#define	WDTYPE_DUALPORTMULTI	2	 /* dual port, multiple sector buffer */
#define	WDTYPE_DUALPORTMULTICACHE 3	 /* above plus track cache */
	ushort	w_cnsbsz;		/* sector buffer size, in sectors */
	ushort	w_necc;			/* ecc bytes appended */
	char	w_rev[8];		/* firmware revision */
	char	w_model[40];		/* model name */
	ushort	w_nsecperint;		/* sectors per interrupt */
	ushort	w_usedmovsd;		/* can use double word read/write? */
};

/*
 * This is our post-massage version of wdparms.  It contains things
 * precalculated into the shape we'll need.
 */
struct wdparms {
	uint w_cyls;		/* = w_fixedcyl+w->w_removcyl */
	uint w_tracks;		/* = w_heads */
	uint w_secpertrk;	/* = w_sectors */
	uint w_secpercyl;	/* = w_heads * w_sectors */
	uint w_size;		/* = w_secpercyl * w_cyls */
};

/*
 * Values for f_nodes
 */
#define ROOTDIR (-1)		/* Root */
#define UNITSTEP (0x10)		/* Node number step between units */ 
#define NODE_UNIT(n) ((n >> 4) & 0xFF)
				/* Unit number */
#define NODE_SLOT(n) (n & 0xF)	/* Partition in unit */
#define MKNODE(unit, slot) ((((unit) & 0xFF) << 4) | (slot))
				/* Merge unit and slot to make node number */
/*
 * State of an open file
 */
struct file {
	long f_sender;		/* Sender of current operation */
	uint f_flags;		/* User access bits */
	struct llist		/* When operation pending on this file */
		*f_list;
	int f_node;		/* Current "directory" */
	ushort f_unit;		/* Unit we want */
	ushort f_abort;		/* Abort requested */
	uint f_blkno;		/* Block # for operation */
	uint f_count;		/* # bytes wanted for current op */
	ushort f_op;		/* FS_READ, FS_WRITE */
	void *f_buf;		/* Base of buffer for operation */
	off_t f_pos;		/* Offset into device */
	int f_local;		/* f_buf is a local buffer */
};

/*
 * State of a disk
 */
struct disk {
	struct part		/* Partition details */
		*d_parts[MAX_PARTS];
	struct wdparms d_parm;	/* Disk "physical" parameters */
	int d_configed;		/* Is the disk configured */
};

/*
 * Function prototypes for wd.c
 */
extern void wd_init(int argc, char **argv);
extern int wd_io(int op, void *handle, uint unit,
		 ulong secnum, void *va, uint secs);
extern void wd_isr(void);

/*
 * Function prototypes for rw.c
 */
extern void wd_rw(struct msg *m, struct file *f);
extern void iodone(void *tran, int result);
extern void rw_init(void);
extern void rw_readpartitions(int unit);

/*
 * Function prototypes for stat.c
 */
extern void wd_stat(struct msg *m, struct file *f);
extern void wd_wstat(struct msg *m, struct file *f);

/*
 * Function prototypes for dir.c
 */
extern void wd_readdir(struct msg *m, struct file *f);
extern void wd_open(struct msg *m, struct file *f);

/*
 * Global data
 */
extern uint first_unit,		/* Lowest unit # configured */
	partundef;		/* All partitioning read yet? */
extern struct disk disks[];
extern struct prot wd_prot;
extern int wd_irq, wd_baseio;
extern port_name wdname;
extern char wd_namer_name[];

#endif /* _WD_H */
@


1.8
log
@Convert to -ldpart
@
text
@d5 1
a5 1
 *	Wester Digital ST-506 type hard disk interface definitions
d11 1
a11 1
#define NWD (2)		/* Max # WD units supported */
d19 1
a19 1
#define SECSZ (512)	/* Only 512 byte sectors handled */
d22 4
a25 4
#define WD_PORT	0x1f0		/* I/O ports here */
#define WD_IRQ	14		/* IRQ # for hard disk */
#define WD_LOW WD_PORT		/* Low/high range */
#define WD_HIGH (WD_PORT+WD_CTLR)
d119 1
d129 3
a131 3
	long f_sender;	/* Sender of current operation */
	uint f_flags;	/* User access bits */
	struct llist	/* When operation pending on this file */
d133 9
a141 9
	int f_node;	/* Current "directory" */
	ushort f_unit;	/* Unit we want */
	ushort f_abort;	/* Abort requested */
	uint f_blkno;	/* Block # for operation */
	uint f_count;	/* # bytes wanted for current op */
	ushort f_op;	/* FS_READ, FS_WRITE */
	void *f_buf;	/* Base of buffer for operation */
	off_t f_pos;	/* Offset into device */
	int f_local;	/* f_buf is a local buffer */
d148 1
a148 3
	struct prot		/* Protection for whole-disk */
		d_prot;
	struct part		/* Partitions */
d150 2
d154 38
a191 4
extern void wd_init(int, char **);
extern void iodone();
extern char configed[];
extern int wd_io(int, void *, uint, ulong, void *, uint);
@


1.7
log
@Source reorg
@
text
@d9 1
a9 1
#include "fdisk.h"
d116 9
a143 21
 * Values for f_node
 */
#define ROOTDIR (-1)				/* Root */
#define NODE_UNIT(n) ((n >> 4) & 0xFF)		/* Unit # */
#define NODE_SLOT(n) (n & 0xF)			/* Partition in unit */
#define WHOLE_DISK (0xF)			/*  ...last is whole disk */
#define MKNODE(unit, slot) ((((unit) & 0xFF) << 4) | (slot))

/*
 * Shape of a partition
 */
struct part {
	char p_name[16];	/* Symbolic name */
	ulong p_off;		/* Sector offset */
	ulong p_len;		/*  ...length */
	int p_val;		/* Valid slot? */
	struct prot		/* Protection for partition */
		p_prot;
};

/*
d150 1
a150 1
		d_parts[NPART];
a152 1
extern int get_offset(int, ulong, ulong *, uint *);
d156 1
@


1.6
log
@Add Pat Mackinlay's patches to (1) show the geometry on boot,
and (2) configure the controller for the geometry, instead
of trusting DOS (which can do some strange things).
@
text
@d9 1
a9 1
#include <wd/fdisk.h>
@


1.5
log
@Add AUTO_HEAD so dumb IDE controllers can work, too
@
text
@d69 1
@


1.4
log
@Allow up to two disks always, add some global prototypes
for better checking of parameters.
@
text
@d13 6
@


1.3
log
@Tweak stuff for IDE drive (SLOW self-test), default is only
one drive, allow 128K I/O for single read of large DOS FAT.
@
text
@d11 1
a11 1
#define NWD (1)		/* Max # WD units supported */
d159 3
@


1.2
log
@Add prototype for get_offset
@
text
@d11 1
a11 1
#define NWD (2)		/* Max # WD units supported */
d14 1
a14 1
#define MAXIO (64*1024)	/* Max I/O--64K */
@


1.1
log
@Initial revision
@
text
@d158 2
@
