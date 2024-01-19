#ifndef _SCREEN_H
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
