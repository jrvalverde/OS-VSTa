head	1.14;
access;
symbols
	V1_3_1:1.12
	V1_3:1.12
	V1_2:1.8
	V1_1:1.8
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.14
date	94.10.06.01.56.15;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.06.21.20.57.06;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.04.11.00.35.47;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.03.04.02.02.21;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.03.01.17.24.19;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.02.28.22.03.58;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.45.20;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.10.14.03.40.13;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.09.19.19.14.23;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.02.20.17.07;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.06.30.19.56.58;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.08.17.49.24;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.08.23.05.02;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.12.19.57.15;	author vandys;	state Exp;
branches;
next	;


desc
@Low-level I/O setup--controller programming and completion
code interpretation
@


1.14
log
@Add support for multiple controllers and such
@
text
@/*
 * wd.c
 *	Western Digital hard disk handling
 *
 * This level of code assumes a single transfer is active at a time.
 * It will handle a contiguous series of sector transfers.
 */
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/assert.h>
#include <sys/syscall.h>
#include <mach/nvram.h>
#include <mach/io.h>
#include <syslog.h>
#include <std.h>
#include <stdio.h>
#include <time.h>
#include "wd.h"

static int wd_cmd(int);
static void wd_start(), wd_readp(int), wd_cmos(int),
	wd_parseparms(int, char *);

uint first_unit;		/* Lowest unit # configured */

/*
 * Miscellaneous counters
 */
ulong wd_strayintr = 0, wd_softerr = 0;

/*
 * Current transfer
 */
static void *busy = 0;	/* Opaque handle from our caller */
static uint cur_unit;	/* Unit active on */
static ulong cur_sec;	/* Sector being transferred now */
static uint cur_xfer;	/*  ...# sectors in this operation */
static uint cur_secs;	/* Sectors left (including cur_sec) */
static char *cur_vaddr;	/* VA to stuff bytes into */
static int cur_op;	/* Current op (FS_READ/FS_WRITE) */

/*
 * swab2()
 *	Swap bytes in a range
 */
#define SWAB(field) swab2(&(field), sizeof(field))
static void
swab2(void *ptr, uint cnt)
{
	char *p, c;
	int x;

	p = ptr;
	for (x = 0; x < cnt; x += 2, p += 2) {
		c = p[0]; p[0] = p[1]; p[1] = c;
	}
}

/*
 * load_sect()
 *	Load a sector down to the controller during an FS_WRITE
 */
static void
load_sect(void)
{
	repoutsw(wd_baseio + WD_DATA, cur_vaddr,
		SECSZ/sizeof(ushort));
	cur_vaddr += SECSZ;
	cur_sec += 1;
	cur_secs -= 1;
	cur_xfer -= 1;
}

/*
 * usage()
 *	Tell how to use
 */
static void
usage(void)
{
	fprintf(stderr, "Usage: wd [ch[1-2] | userdef] <drive-configs>" \
			" [opts=args] ...\n\n");
	fprintf(stderr, "       drive-configs: d[number]:readp\n" \
			"                      d[number]:cmos\n" \
			"                      d[number]:[cyls]:" \
			"[tracks]:[sectors]\n\n");
	fprintf(stderr, "       options:       iobase=<I/O-base-address>\n" \
			"                      irq=<IRQ-number>\n" \
			"                      namer=<namer-entry>\n\n");
	exit(1);
}

/*
 * wd_init()
 *	Initialize our disk controller
 *
 * Initialize controller.
 *
 * For each disk unit, see if it's present by trying to read its
 * parameters.
 */
void
wd_init(int argc, char **argv)
{
	int i = 1;
	int x;
	int found_first = 0;
	int cfg_params[NWD] = {0, 0};
	char *check;

	/*
	 * Start checking the usage
	 */
	if (argc < 2) {
		usage();
	}
	if (!strncmp(argv[1], "ch", 2)) {
		int p;

		/*
		 * We're using a standard disk definition - find the lookup
		 * table reference details
		 */		
		p = argv[1][2] - '1';
		if (p < 0 || p >= NWD) {
			usage();
		}
		if (p == 0) {
			wd_baseio = WD1_PORT;
			wd_irq = WD1_IRQ;
			strcpy(wd_namer_name, "disk/wd1");
		} else {
			wd_baseio = WD2_PORT;
			wd_irq = WD2_IRQ;
			strcpy(wd_namer_name, "disk/wd2");
		}
		i++;
	} else if (!strcmp(argv[1], "userdef")) {
		/*
		 * Set initial conditions for a user-defined setup.  Basically
		 * make sure that there are no defaults
		 */
		wd_baseio = 0;
		wd_irq = 0;
		i++;
	} else {
		/*
		 * Establish the default
		 */
		wd_baseio = WD1_PORT;
		wd_irq = WD1_IRQ;
		strcpy(wd_namer_name, "disk/wd");
	}

	/*
	 * Start processing the option parameters
	 */
	while (i < argc) {
		if (argv[i][0] == 'd') {
			/*
			 * Select drive parameters
			 */
			int n;
			
			n = argv[i][1] - '0';
			if (n < 0 || n >= NWD) {
				fprintf(stderr, "wd: non supported drive " \
					"number '%c' - aborting\n",
					argv[i][1]);	
				exit(1);
			}
			cfg_params[n] = i;
		} else if (!strncmp(argv[i], "irq=", 4)) {
			/*
			 * Select a new IRQ line
			 */
			wd_irq = (int)strtol(&argv[i][4], &check, 0);
			if (check == &argv[i][4] || *check != '\0') {
				fprintf(stderr, "wd: invalid IRQ setting " \
					"'%s' - aborting\n", argv[i]);
				exit(1);
			}
		} else if (!strncmp(argv[i], "baseio=", 7)) {
			/*
			 * Select a new base I/O port address
			 */
			wd_baseio = (int)strtol(&argv[i][7], &check, 0);
			if (check == &argv[i][7] || *check != '\0') {
				fprintf(stderr, "wd: invalid I/O adress " \
					"'%s' - aborting\n", argv[i]);
				exit(1);
			}
		} else if (!strncmp(argv[i], "namer=", 6)) {
			/*
			 * Select a new namer entry
			 */
			if ((strlen(&argv[i][6]) == 0)
			    || (strlen(&argv[i][6]) >= NAMESZ)) {
				fprintf(stderr, "wd: invalid name '%s' " \
					"- aborting\n", &argv[i][6]);
				exit(1);
			}
			strcpy(wd_namer_name, &argv[i][6]);
		} else {
			fprintf(stderr,
				"wd: unknown option '%s' - aborting\n",
				argv[i]);
			exit(1);
		}
		i++;
	}

	/*
	 * Check that after all of the messing about we have a valid set
	 * of parameters - report failures if we don't
	 */
	if (wd_baseio == 0) {
		fprintf(stderr,
			"wd: no I/O base address specified - aborting\n");
		exit(1);
	}
	if (wd_irq == 0) {
		fprintf(stderr,
			"wd: no IRQ line specified - aborting\n");
		exit(1);
	}
	if (wd_namer_name[0] == '\0') {
		fprintf(stderr,
			"wd: no namer entry specified - aborting\n");
		exit(1);
	}

	/*
	 * Enable I/O for the needed range
	 */
	if (enable_io(wd_baseio, wd_baseio + WD_CTLR) < 0) {
		syslog(LOG_ERR, "I/O permissions not granted");
		exit(1);
	}

	/*
	 * Send reset to controller, wait, drop reset bit.
	 */
	outportb(wd_baseio + WD_CTLR, CTLR_RESET|CTLR_IDIS);
	__msleep(100);
	outportb(wd_baseio + WD_CTLR, CTLR_IDIS);
	__msleep(100);

	/*
	 * Ask him if he's OK
	 */
	if (wd_cmd(WDC_DIAG) < 0) {
		syslog(LOG_ERR, "controller failed self diagnostic");
		exit(1);
	}
	inportb(wd_baseio + WD_ERROR);

	/*
	 * Allow interrupts now
	 */
	outportb(wd_baseio + WD_CTLR, CTLR_4BIT);

	/*
	 * Scan the unit whose parameters were specified
	 */
	for (i = 0; i < NWD; i++) {
		disks[i].d_configed = 0;
		x = cfg_params[i];
		if (!x) {
			continue;
		}

		/*
		 * Verify colon and argument
		 */
		if ((argv[x][2] != ':') || !argv[x][3]) {
			fprintf(stderr,
				"wd: bad argument '%s' - aborting\n",
				argv[cfg_params[i]]);
			exit(1);
		}

		/*
		 * Now get drive parameters in the requested way
		 */
		if (!strcmp(argv[x]+3, "readp")) {
			wd_readp(i);
		} else if (!strcmp(argv[x]+3, "cmos")) {
			wd_cmos(i);
		} else {
			wd_parseparms(i, argv[x]+3);
		}

		if (disks[i].d_configed) {
			struct wdparms *w = &disks[i].d_parm;
			uint s = w->w_size * SECSZ;
			const uint m = 1024*1024;

			syslog(LOG_INFO,
				"unit %d: %d.%dM - " \
				"%d heads, %d cylinders, %d sectors",
				i, s / m, (s % m) / (m / 10),
				w->w_tracks, w->w_cyls, w->w_secpertrk);
			found_first = 1;
			if (i < first_unit) {
				first_unit = i;
			}
		}
	}
	if (!found_first) {
		syslog(LOG_ERR, "no units found, exiting");
		exit(1);
	}
}

/*
 * wd_io()
 *	Entry into machinery for doing I/O to a disk
 *
 * Validity of count and alignment have already been checked.
 * Interpretation of partitioning, if any, has also already been
 * done by our caller.  This routine and below looks at the disk
 * as a pure, 0-based array of SECSZ blocks.
 *
 * Returns 0 on successfully initiated I/O, 1 on error.
 */
int
wd_io(int op, void *handle, uint unit, ulong secnum, void *va, uint secs)
{
	ASSERT_DEBUG(unit < NWD, "wd_io: bad unit");
	ASSERT_DEBUG(busy == 0, "wd_io: busy");
	ASSERT_DEBUG(handle != 0, "wd_io: null handle");
	ASSERT_DEBUG(secs > 0, "wd_io: 0 len");

	/*
	 * If not configured, error out
	 */
	if (!disks[unit].d_configed) {
		return(1);
	}
	ASSERT_DEBUG(secnum < disks[unit].d_parm.w_size, "wd_io: high sector");

	/*
	 * Record transfer parameters
	 */
	busy = handle;
	cur_unit = unit;
	cur_sec = secnum;
	cur_secs = secs;
	cur_vaddr = va;
	cur_op = op;

	/*
	 * Kick off first transfer
	 */
	wd_start();
	return(0);
}

/*
 * wd_start()
 *	Given I/O described by global parameters, initiate next sector I/O
 *
 * The size of the I/O will be the lesser of the amount left on the track
 * and the amount requested.  cur_xfer will be set to this count, in units
 * of sectors.
 */
static void
wd_start(void)
{
	uint cyl, sect, trk, lsect;
	struct wdparms *w = &disks[cur_unit].d_parm;

#ifdef DEBUG
	ASSERT((inportb(wd_baseio + WD_STATUS) & WDS_BUSY) == 0,
		"wd_start: busy");
#endif
	/*
	 * Given disk geometry, calculate parameters for next I/O
	 */
	cyl =  cur_sec / w->w_secpercyl;
	sect = cur_sec % w->w_secpercyl;
#ifdef AUTO_HEAD
	lsect = w->w_secpercyl - sect;
#endif
	trk = sect / w->w_secpertrk;
	sect = (sect % w->w_secpertrk) + 1;
#ifndef AUTO_HEAD
	/* Cap at end of this track--heads won't switch automatically */
	lsect = w->w_secpertrk + 1 - sect;
#endif

	/*
	 * Transfer size--either the rest, or the remainder of this track
	 */
	if (cur_secs > lsect) {
		cur_xfer = lsect;
	} else {
		cur_xfer = cur_secs;
	}

	/*
	 * Program I/O
	 */
	outportb(wd_baseio + WD_SCNT, cur_xfer);
	outportb(wd_baseio + WD_SNUM, sect);
	outportb(wd_baseio + WD_CYL0, cyl & 0xFF);
	outportb(wd_baseio + WD_CYL1, (cyl >> 8) & 0xFF);
	outportb(wd_baseio + WD_SDH,
		WDSDH_EXT|WDSDH_512 | trk | (cur_unit << 4));
	outportb(wd_baseio + WD_CMD,
		(cur_op == FS_READ) ? WDC_READ : WDC_WRITE);

	/*
	 * Feed data immediately for write
	 */
	if (cur_op == FS_WRITE) {
		while ((inportb(wd_baseio + WD_STATUS) & WDS_DRQ) == 0) {
			;
		}
		load_sect();
	}
}

/*
 * wd_isr()
 *	Called on interrupt to the WD IRQ
 */
void
wd_isr(void)
{
	uint stat;
	int done = 0;

	/*
	 * Get status.  If this interrupt was meaningless, just
	 * log it and return.
	 */
	stat = inportb(wd_baseio + WD_STATUS);
	if ((stat & WDS_BUSY) || !busy) {
		wd_strayintr += 1;
		return;
	}

	/*
	 * Error--abort current activity, log error
	 */
	if (stat & WDS_ERROR) {
		void *v;

		syslog(LOG_ERR, "hard error unit %d sector %d error=0x%x",
		       cur_unit, cur_sec, inportb(wd_baseio + WD_ERROR));
		v = busy;
		busy = 0;
		iodone(v, -1);
		return;
	}

	/*
	 * Quietly tally soft errors
	 */
	if (stat & WDS_ECC) {
		wd_softerr += 1;
	}

	/*
	 * Read in or write out next sector
	 */
	if (cur_op == FS_READ) {
		/*
		 * Sector is ready; read it in and advance counters
		 */
		repinsw(wd_baseio + WD_DATA, cur_vaddr, SECSZ/sizeof(ushort));
		cur_vaddr += SECSZ;
		cur_sec += 1;
		cur_secs -= 1;
		cur_xfer -= 1;
	} else {
		/*
		 * Writes are in two phases; first we are interrupted
		 * to provide data, then we're interrupted when the
		 * data is written.
		 */
		if (stat & WDS_DRQ) {
			load_sect();
		} else {
			done = 1;
		}
	}

	/*
	 * Done with current transfer--complete I/O, or start next part
	 */
	if (((cur_op == FS_READ) && (cur_xfer == 0)) || done) {
		if (cur_secs > 0) {
			/*
			 * This will calculate next step, and fire
			 * the I/O.
			 */
			wd_start();
		} else {
			void *v = busy;

			/*
			 * All done.  Let upper level know.
			 */
			busy = 0;
			iodone(v, 0);
		}
	}
}

/*
 * wd_cmd()
 *	Send a command and wait for controller acceptance
 *
 * Returns -1 on error, otherwise value of status byte.
 */
static int
wd_cmd(int cmd)
{
	uint count;
	int stat;
	const uint timeout = 10000000;

	/*
	 * Wait for controller to be ready
	 */
	count = timeout;
	while (inportb(wd_baseio + WD_STATUS) & WDS_BUSY) {
		if (--count == 0) {
			return(-1);
		}
	}

	/*
	 * Send command, wait for controller to finish
	 */
	outportb(wd_baseio + WD_CMD, cmd);
	count = timeout;
	for (;;) {
		stat = inportb(wd_baseio + WD_STATUS);
		if ((stat & WDS_BUSY) == 0) {
			return(stat);
		}
		if (--count == 0) {
			return(-1);
		}
	}
}

/*
 * readp_data()
 *	After sending a READP command, wait for data and place in buffer
 */
static int
readp_data(void)
{
	uint count;

	/*
	 * Wait for data or error
	 */
	for (count = 200000; count > 0; --count) {
		uint stat;

		stat = inportb(wd_baseio + WD_STATUS);
		if (stat & WDS_ERROR) {
			return(-1);
		}
		if (stat & WDS_DRQ) {
			return(stat);
		}
	}
	return(-1);
}

/*
 * wd_readp()
 *	Issue READP to drive, get its geometry and such
 *
 * On success, sets disks[unit].d_configed to 1.
 */
static void
wd_readp(int unit)
{
	char buf[SECSZ];
	struct wdparms *w;
	struct wdparameters xw;

	/*
	 * Send READP and see if he'll answer
	 */
	outportb(wd_baseio + WD_SDH, WDSDH_EXT|WDSDH_512 | (unit << 4));
	if (wd_cmd(WDC_READP) < 0) {
		return;
	}

	/*
	 * Read in the parameters
	 */
	if (readp_data() < 0) {
		return;
	}
	repinsw(wd_baseio + WD_DATA, buf, sizeof(buf)/sizeof(ushort));
	bcopy(buf, &xw, sizeof(xw));

	/*
	 * Give the controller the geometry. I'm not really sure why my
	 * drive needs the little delay, but it did... (pat)
	 */
	__msleep(100);
	outportb(wd_baseio + WD_SDH,
		WDSDH_EXT|WDSDH_512 | (unit << 4) | (xw.w_heads - 1));
	outportb(wd_baseio + WD_SCNT, xw.w_sectors);
	if (wd_cmd(WDC_SPECIFY) < 0) {
		return;
	}

	/*
	 * Fix big-endian lossage
	 */
	SWAB(xw.w_model);

	/*
	 * Massage into a convenient format
	 */
	w = &disks[unit].d_parm;
	w->w_cyls = xw.w_fixedcyl + xw.w_removcyl;
	w->w_tracks = xw.w_heads;
	w->w_secpertrk = xw.w_sectors;
	w->w_secpercyl = xw.w_heads * xw.w_sectors;
	w->w_size = w->w_secpercyl * w->w_cyls;
	disks[unit].d_configed = 1;
}

/*
 * wd_parseparms()
 *	Parse user-specified drive parameters
 */
static void
wd_parseparms(int unit, char *parms)
{
	struct wdparms *w = &disks[unit].d_parm;

	if (sscanf(parms, "%d:%d:%d", &w->w_cyls, &w->w_tracks,
			&w->w_secpertrk) != 3) {
		syslog(LOG_ERR, "unit %d: bad parameters: %s",
		       unit, parms);
		return;
	}
	w->w_secpercyl = w->w_tracks * w->w_secpertrk;
	w->w_size = w->w_secpercyl * w->w_cyls;
	disks[unit].d_configed = 1;
}

/*
 * cmos_read()
 *	Read a byte through the NVRAM interface
 */
static uint
cmos_read(unsigned char address)
{
	outportb(RTCSEL, address);
	return(inportb(RTCDATA));
}

/*
 * wd_cmos()
 *	Get disk parameters from NVRAM BIOS storage
 */
static void
wd_cmos(int unit)
{
	const uint
		nv_cfg = 0x12,		/* Config'ed at all */
		nv_lo8 = 0x1B,		/* Low, high count of secs */
		nv_hi8 = 0x1C,
		nv_heads = 0x1D,	/* # heads */
		nv_sec = 0x23;		/* # sec/track */
	uint off =			/* I/O off for unit */
		((unit == 0) ? 0 : 9);
	struct wdparms *w;

	/*
	 * NVRAM only handles two
	 */
	if (unit > 1) {
		syslog(LOG_ERR, "unit %d: no CMOS information", unit);
		return;
	}

	/*
	 * Read config register to see if present
	 */
	if ((cmos_read(nv_cfg) & ((unit == 0) ? 0xF0 : 0x0F)) == 0) {
		return;
	}

	/*
	 * It appears present, so read in the parameters
	 */
	w = &disks[unit].d_parm;
	w->w_cyls = cmos_read(nv_hi8 + off) << 8 | cmos_read(nv_lo8 + off);
	w->w_tracks = cmos_read(nv_heads + off);
	w->w_secpertrk = cmos_read(nv_sec + off);

	/*
	 * Some NVRAM setups will have the drive marked present,
	 * but with zeroes for all parameters.  Pretend like it
	 * isn't there at all.
	 */
	if (!w->w_cyls || !w->w_tracks || !w->w_secpertrk) {
		return;
	}

	/*
	 * Otherwise calculate the reset & mark it present
	 */
	w->w_secpercyl = w->w_tracks * w->w_secpertrk;
	w->w_size = w->w_secpercyl * w->w_cyls;
	disks[unit].d_configed = 1;
}
@


1.13
log
@Convert to openlog()
@
text
@d3 1
a3 1
 *	Wester Digital hard disk handling
d11 1
a26 7
 * The parameters we read on each disk, and a flag to ask if we've
 * gotten them yet.
 */
struct wdparms parm[NWD];
char configed[NWD];

/*
d66 1
a66 1
	repoutsw(WD_PORT+WD_DATA, cur_vaddr,
d75 19
d105 1
d107 133
a239 1
	int found_first;
d244 1
a244 1
	outportb(WD_PORT+WD_CTLR, CTLR_RESET|CTLR_IDIS);
d246 1
a246 1
	outportb(WD_PORT+WD_CTLR, CTLR_IDIS);
d253 1
a253 1
		syslog(LOG_ERR, "controller fails diagnostic\n");
d256 1
a256 1
	inportb(WD_PORT+WD_ERROR);
d261 1
a261 1
	outportb(WD_PORT+WD_CTLR, CTLR_4BIT);
d264 1
a264 1
	 * First, mark nothing as found
d266 4
a269 23
	found_first = 0;
	bzero(configed, sizeof(configed));

	/*
	 * Scan units
	 */
	for (x = 1; x < argc; ++x) {
		uint unit;

		/*
		 * Sanity check command line format
		 */
		if (argv[x][0] != 'd') {
			syslog(LOG_ERR, "bad arg: %s\n", argv[x]);
			continue;
		}

		/*
		 * Sanity check drive number
		 */
		unit = argv[x][1] - '0';
		if (unit >= NWD) {
			syslog(LOG_ERR, "bad drive: %s\n", argv[x]);
d277 4
a280 2
			syslog(LOG_ERR, "bad arg: %s\n", argv[x]);
			continue;
d287 1
a287 1
			wd_readp(unit);
d289 1
a289 1
			wd_cmos(unit);
d291 1
a291 1
			wd_parseparms(unit, argv[x]+3);
d294 3
a296 2
		if (configed[unit]) {
			uint s = parm[unit].w_size * SECSZ;
d300 4
a303 3
"unit %d: %d.%dM - %d heads, %d cylinders, %d sectors\n",
	unit, s / m, (s % m) / (m/10),
	parm[unit].w_tracks, parm[unit].w_cyls, parm[unit].w_secpertrk);
d305 2
a306 2
			if (unit < first_unit) {
				first_unit = unit;
d311 1
a311 1
		syslog(LOG_ERR, "no units found, exiting.\n");
d338 1
a338 1
	if (!configed[unit]) {
d341 1
a341 1
	ASSERT_DEBUG(secnum < parm[unit].w_size, "wd_io: high sector");
d372 1
a372 1
	struct wdparms *w = &parm[cur_unit];
d375 1
a375 1
	ASSERT((inportb(WD_PORT+WD_STATUS) & WDS_BUSY) == 0,
d405 5
a409 5
	outportb(WD_PORT+WD_SCNT, cur_xfer);
	outportb(WD_PORT+WD_SNUM, sect);
	outportb(WD_PORT+WD_CYL0, cyl & 0xFF);
	outportb(WD_PORT+WD_CYL1, (cyl >> 8) & 0xFF);
	outportb(WD_PORT+WD_SDH,
d411 1
a411 1
	outportb(WD_PORT+WD_CMD,
d418 1
a418 1
		while ((inportb(WD_PORT+WD_STATUS) & WDS_DRQ) == 0) {
d439 1
a439 1
	stat = inportb(WD_PORT+WD_STATUS);
d451 2
a452 2
		syslog(LOG_ERR, "hard error unit %d sector %d error=0x%x\n",
		       cur_unit, cur_sec, inportb(WD_PORT+WD_ERROR));
d473 1
a473 1
		repinsw(WD_PORT+WD_DATA, cur_vaddr, SECSZ/sizeof(ushort));
d530 1
a530 1
	while (inportb(WD_PORT + WD_STATUS) & WDS_BUSY) {
d539 1
a539 1
	outportb(WD_PORT + WD_CMD, cmd);
d542 1
a542 1
		stat = inportb(WD_PORT + WD_STATUS);
d567 1
a567 1
		stat = inportb(WD_PORT + WD_STATUS);
d582 1
a582 1
 * On success, sets configed[unit] to 1.
d594 1
a594 1
	outportb(WD_PORT+WD_SDH, WDSDH_EXT|WDSDH_512 | (unit << 4));
d605 1
a605 1
	repinsw(WD_PORT+WD_DATA, buf, sizeof(buf)/sizeof(ushort));
d613 1
a613 1
	outportb(WD_PORT+WD_SDH,
d615 1
a615 1
	outportb(WD_PORT+WD_SCNT, xw.w_sectors);
d628 1
a628 1
	w = &parm[unit];
d634 1
a634 1
	configed[unit] = 1;
d644 1
a644 1
	struct wdparms *w = &parm[unit];
d648 2
a649 2
		syslog(LOG_ERR, "unit %d: bad parameters: %s\n",
			unit, parms);
d654 1
a654 1
	configed[unit] = 1;
d689 1
a689 1
		syslog(LOG_ERR, "unit %d: no CMOS information\n", unit);
d703 1
a703 1
	w = &parm[unit];
d722 1
a722 1
	configed[unit] = 1;
@


1.12
log
@Fix warnings
@
text
@d107 1
a107 1
		syslog(LOG_ERR, "wd: controller fails diagnostic\n");
d133 1
a133 1
			syslog(LOG_ERR, "wd: bad arg: %s\n", argv[x]);
d142 1
a142 1
			syslog(LOG_ERR, "wd: bad drive: %s\n", argv[x]);
d150 1
a150 1
			syslog(LOG_ERR, "wd: bad arg: %s\n", argv[x]);
d170 1
a170 1
"wd%d: %d.%dM - %d heads, %d cylinders, %d sectors\n",
d180 1
a180 1
		syslog(LOG_ERR, "wd: no units found, exiting.\n");
d320 1
a320 1
		syslog(LOG_ERR, "wd: hard error unit %d sector %d error=0x%x\n",
d517 2
a518 1
		syslog(LOG_ERR, "wd%d: bad parameters: %s\n", parms);
d558 1
a558 2
		syslog(LOG_ERR,
		       "wd%d: only 0 and 1 have CMOS information\n", unit);
@


1.11
log
@Convert to -ldpart
@
text
@d49 1
a49 1
 * swab()
d52 1
a52 1
#define SWAB(field) swab(&(field), sizeof(field))
d54 1
a54 1
swab(void *ptr, uint cnt)
@


1.10
log
@Flag info as info, not err
@
text
@d12 1
d14 3
d107 1
a107 1
		syslog(LOG_ERR, "WD controller fails diagnostic\n");
d196 1
d321 1
a321 1
			cur_unit, cur_sec, inportb(WD_PORT+WD_ERROR));
d388 1
a388 1
static
d425 1
a425 1
static
a428 1
	int stat;
d433 3
a435 1
	for (;;) {
a442 3
		if (--count == 0) {
			return(-1);
		}
d444 1
d558 1
a558 1
			"wd%d: only 0 and 1 have CMOS information\n", unit);
@


1.9
log
@Convert to syslog()
@
text
@d165 1
a165 1
			syslog(LOG_ERR,
@


1.8
log
@Source reorg
@
text
@d12 1
d19 1
a19 1
uint first_unit;		/* Lowerst unit # configured */
d103 1
a103 1
		printf("WD controller fails diagnostic\n");
d129 1
a129 1
			printf("wd: bad arg: %s\n", argv[x]);
d138 1
a138 1
			printf("wd: bad drive: %s\n", argv[x]);
d146 1
a146 1
			printf("wd: bad arg: %s\n", argv[x]);
d165 1
a165 1
			printf(
d176 1
a176 1
		printf("wd: no units found, exiting.\n");
d315 1
a315 1
		printf("wd: hard error unit %d sector %d error=0x%x\n",
d513 1
a513 1
		printf("wd%d: bad parameters: %s\n", parms);
d553 2
a554 1
		printf("wd%d: only 0 and 1 have CMOS information\n", unit);
@


1.7
log
@Add Pat Mackinlay's patches to (1) show the geometry on boot,
and (2) configure the controller for the geometry, instead
of trusting DOS (which can do some strange things).
@
text
@a9 1
#include <wd/wd.h>
d12 1
@


1.6
log
@Add AUTO_HEAD so dumb IDE controllers can work, too
@
text
@d164 4
a167 2
			printf("wd%d: %d.%dM\n", unit,
				s / m, (s % m) / (m/10));
d471 12
@


1.5
log
@Massive changes to allow disk parms from (1) readp (the original
way), (2) NVRAM (thanks to Patrick Mackinlay), and (3) hand-
entered (thanks again to Patrick!)
@
text
@d244 1
d246 1
d249 4
@


1.4
log
@Tweak stuff for IDE drive (SLOW self-test), default is only
one drive, allow 128K I/O for single read of large DOS FAT.
@
text
@d12 1
d14 3
a16 3
static int wd_readp(), wd_cmd();
static void wd_start();
extern void iodone();
a23 1
static struct wdparameters xparm[NWD];
d25 1
a25 1
int configed[NWD];
d85 1
a85 1
wd_init(void)
d113 6
d121 35
a155 4
	found_first = 0;
	for (x = 0; x < NWD; ++x) {
		if (wd_readp(x) < 0) {
			configed[x] = 0;
d157 5
a161 1
			uint s = parm[x].w_size * SECSZ;
d164 1
a164 1
			printf("wd%d: %d.%dM\n", x,
d166 3
a168 4
			configed[x] = 1;
			if (!found_first) {
				found_first = 1;
				first_unit = x;
d437 2
d440 1
a440 1
static
d445 1
a445 1
	struct wdparameters *xw;
d452 1
a452 1
		return(-1);
d459 1
a459 1
		return(-1);
d462 1
a462 2
	xw = &xparm[unit];
	bcopy(buf, xw, sizeof(*xw));
d467 1
a467 1
	SWAB(xw->w_model);
d473 91
a563 4
	w->w_cyls = xw->w_fixedcyl + xw->w_removcyl;
	w->w_tracks = xw->w_heads;
	w->w_secpertrk = xw->w_sectors;
	w->w_secpercyl = xw->w_heads * xw->w_sectors;
d565 1
@


1.3
log
@Unify units of device size during autoconf
@
text
@d339 1
d344 1
a344 1
	count = 100000;
d355 1
a355 1
	count = 100000;
@


1.2
log
@Fix first-sector handling for transfers; enable writes.
Writes appear to work!
@
text
@d120 5
a124 1
			printf("wd%d: %d sectors\n", x, parm[x].w_size);
@


1.1
log
@Initial revision
@
text
@d61 15
a193 2
	/* XXX for now, would be a pain to fix */
	ASSERT(cur_op == FS_READ, "wd_start: writing");
d224 10
d296 1
a296 6
			repoutsw(WD_PORT+WD_DATA, cur_vaddr,
				SECSZ/sizeof(ushort));
			cur_vaddr += SECSZ;
			cur_sec += 1;
			cur_secs -= 1;
			cur_xfer -= 1;
@
