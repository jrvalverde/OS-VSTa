head	1.7;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.7
date	94.04.07.00.48.59;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.04.17.01.44;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.02.28.19.15.59;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.02.28.05.00.26;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.24.23.01.24;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.18.16.27;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.42.44;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for console state
@


1.7
log
@Add support for -mono and -color
@
text
@#ifndef _SCREEN_H
#define _SCREEN_H
/*
 * screen.h
 *	Common definitions for PC/AT console display
 */
#include <sys/fs.h>
#include <sys/perm.h>
#include <llist.h>
#include <mach/kbd.h>

/*
 * An open file
 */
struct file {
	uint f_gen;		/* Generation of access */
	uint f_flags;		/* User access bits */
	ushort f_pos;		/* For walking virtual dir */
	ushort f_screen;	/* Which virtual screen they're on */
	uint f_readcnt;		/* # bytes requested for current op */
	long f_sender;		/*  ...return addr for current op */
};

/*
 * Per-virtual screen state
 */
struct screen {
	uint s_gen;		/* Generation of access for this screen */
	char *s_img;		/* In-RAM image of display */
	uint s_pos;		/*  ...cursor position in this image */
	char *s_curimg;		/* Current display--s_img, or the HW */
	struct llist		/* Queue of reads pending */
		s_readers;
	char			/* Typeahead */
		s_buf[KEYBD_MAXBUF];
	ushort s_hd, s_tl;	/*  ...circularly buffered */
	uint s_nbuf;		/*  ...amount buffered */
};

/*
 * PC screen attributes
 */
#define NORMAL 7		/* Attribute for normal characters */
#define BLANKW (0x0720)		/* Normal attribute, blank char */
#define BLANK (0x07200720)	/*  ... two of them in a longword */

#define ROWS 25		/* Screen dimensions */
#define COLS 80
#define CELLSZ 2	/* Bytes per character cell */
#define TABS 8		/* Tab stops every 8 positions */
#define SCREENMEM (ROWS*COLS*CELLSZ)
#define NVTY 8		/* # virtual screens supported */

#define ROOTDIR NVTY	/* Special screen # for root dir */

/*
 * Top of respective adaptors
 */
#define CONS_LOW MGAIDX

#define MGATOP 0xB0000
#define MGAIDX 0x3B4
#define MGADAT 0x3B5
#define CGATOP 0xB8000
#define CGAIDX 0x3D4
#define CGADAT 0x3D5

#define CONS_HIGH CGADAT

/*
 * Types of adaptor supported
 */
#define VID_MGA 0
#define VID_CGA 1

/*
 * Shared routines
 */
extern void save_screen(struct screen *), load_screen(struct screen *),
	set_screen(char *, uint), cursor(void),
	save_screen_pos(struct screen *);
extern void select_screen(uint);
extern void write_string(char *, uint), init_screen(int);
extern void cons_stat(struct msg *, struct file *),
	cons_wstat(struct msg *, struct file *);
extern void kbd_isr(struct msg *);
extern void kbd_enqueue(struct screen *, uint);
extern void kbd_read(struct msg *, struct file *);
extern void abort_read(struct file *);
extern void do_dbg_enter(void);
extern void clear_screen(char *);

/*
 * Shared data
 */
extern char *hw_screen;
extern struct prot cons_prot;
extern struct screen screens[];
extern uint accgen;
extern uint curscreen, hwscreen;

#endif /* _SCREEN_H */
@


1.6
log
@Get multiple active I/O screens working
@
text
@d71 1
a71 1
 * Which one we're using
d73 2
a74 9
#ifdef CGA
#define DISPLAY CGATOP
#define IDX CGAIDX
#define DAT CGADAT
#else
#define DISPLAY MGATOP
#define IDX MGAIDX
#define DAT MGADAT
#endif
d83 1
a83 1
extern void write_string(char *, uint), init_screen(void);
@


1.5
log
@Fix syntax errors
@
text
@d107 1
a107 1
extern uint curscreen;
@


1.4
log
@convert for virtual consoles; fix -Wall warnings
@
text
@d10 1
d21 1
d34 1
a34 1
	static char		/* Typeahead */
d36 1
a36 2
	static ushort s_hd,	/*  ...circularly buffered */
		s_tl;
d96 3
@


1.3
log
@Add fansi extended commands--cleol, cleos
@
text
@d7 3
a9 1
#include <sys/types.h>
d15 5
a19 2
	uint f_gen;	/* Generation of access */
	uint f_flags;	/* User access bits */
d22 20
d50 4
d81 23
@


1.2
log
@Add support for tabs
@
text
@d18 2
a19 2
#define BLANK (0x07200720)	/* Normal attribute, blank char */
				/*  ... two of them in a longword */
@


1.1
log
@Initial revision
@
text
@d24 1
@
