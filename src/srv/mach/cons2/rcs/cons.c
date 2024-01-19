head	1.12;
access;
symbols
	V1_3_1:1.11
	V1_3:1.11
	V1_2:1.6
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.12
date	94.09.07.19.22.30;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.04.07.00.48.59;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.03.04.19.22.12;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.03.04.17.01.44;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.02.28.19.16.15;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.02.28.04.52.00;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.45.50;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.24.23.01.24;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.24.21.05.08;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.19.00.57.40;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.18.16.27;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.42.37;	author vandys;	state Exp;
branches;
next	;


desc
@Routines for painting screen
@


1.12
log
@Mistake in moving screen cells around; they're two
bytes each
@
text
@/*
 * cons.c
 *	Routines for serving a console display device
 */
#include <sys/types.h>
#include "cons.h"
#include <sys/mman.h>
#include <std.h>
#include <ctype.h>
#include <mach/io.h>
#include <sys/assert.h>

/*
 * Parameters for screen, filled in by init_screen()
 */
static char *top, *bottom, *cur, *lastl;
char *hw_screen;
static int idx, dat, display;

/*
 * load_screen()
 *	Switch to new screen image as stored in "s"
 */
void
load_screen(struct screen *s)
{
	bcopy(s->s_img, hw_screen, SCREENMEM);
	set_screen(hw_screen, s->s_pos);
	s->s_curimg = hw_screen;
	cursor();
}

/*
 * save_screen()
 *	Dump screen image to memory in "s"
 */
void
save_screen(struct screen *s)
{
	bcopy(hw_screen, s->s_img, SCREENMEM);
}

/*
 * save_screen_pos()
 *	Save just cursor position
 */
void
save_screen_pos(struct screen *s)
{
	s->s_pos = cur-top;
}

/*
 * set_screen()
 *	Cause the emulator to start using the named memory as the display
 */
void
set_screen(char *p, uint cursor)
{
	top = p;
	bottom = p + SCREENMEM;
	lastl = p + ((ROWS-1)*COLS*CELLSZ);
	cur = p + cursor;
	ASSERT_DEBUG((cur >= top) && (cur < bottom), "set_screen: bad cursor");
}

/*
 * cursor()
 *	Take current data position, update hardware cursor to match
 */
void
cursor(void)
{
	ulong pos = (cur-top) >> 1;

	outportb(idx, 0xE);
	outportb(dat, (pos >> 8) & 0xFF);
	outportb(idx, 0xF);
	outportb(dat, pos & 0xFF);
}

/*
 * clear_screen()
 *	Apply unenhanced blanks to all of a screen
 */
void
clear_screen(char *p)
{
	ulong bl, *u, *bot;

	bl = BLANK;
	bot = (ulong *)(p+SCREENMEM);
	for (u = (ulong *)p; u < bot; ++u) {
		*u = bl;
	}
}

/*
 * cls()
 *	Clear screen, home cursor
 */
static void
cls(void)
{
	clear_screen(top);
	cur = top;
}

/*
 * init_screen()
 *	Set up mapping of PC screen
 *
 * We accept either VID_MGA (mono) or VID_CGA (colour) for the type field
 */
void
init_screen(int type)
{
	char *p;

	/*
	 * Establish the video adaptor parameters
	 */
	if (type == VID_MGA) {
		idx = MGAIDX;
		dat = MGADAT;
		display = MGATOP;
	} else {
		idx = CGAIDX;
		dat = CGADAT;
		display = CGATOP;
	}

	/*
	 * Open physical device, make it the current display
	 * destination.
	 */
	p = mmap((void *)display, SCREENMEM, 
		 PROT_READ|PROT_WRITE, MAP_PHYS, 0, 0L);
	if (!p) {
		exit(1);
	}
	hw_screen = p;
	set_screen(p, 0);
	cls(); cursor();
}

/*
 * scrollup()
 *	Scroll the screen up a line, blank last line
 */
static void
scrollup()
{
	ulong *u, bl;

	bcopy(top+(COLS*CELLSZ), top, (ROWS-1)*COLS*CELLSZ);
	bl = BLANK;
	for (u = (ulong *)lastl; u < (ulong *)bottom; ++u) {
		*u = bl;
	}
}

/*
 * blankline()
 *	Blank line starting at "p"
 */
static void
blankline(void *p)
{
	ulong *q = p;
	uint x;

	for (x = 0; x < (COLS*CELLSZ/sizeof(long)); ++x) {
		*q++ = BLANK;
	}
}

/*
 * sequence()
 *	Called when we've decoded args and it's time to act
 */
static void
sequence(int x, int y, char c)
{
	char *p;

	/*
	 * Cap/sanity
	 */
	if (x > COLS) {
		x = COLS;
	}

	/*
	 * Dispatch command
	 */
	switch (c) {
	case 'A':	/* Cursor up */
		while (x-- > 0) {
			cur -= (COLS*CELLSZ);
			if (cur < top) {
				cur += (COLS*CELLSZ);
				x = 0;
			}
		}
		return;
	case 'B':	/* Cursor down a line */
		while (x-- > 0) {
			cur += (COLS*CELLSZ);
			if (cur >= bottom) {
				cur -= (COLS*CELLSZ);
				x = 0;
			}
		}
		return;
	case 'C':	/* Cursor right */
		while (x-- > 0) {
			cur += CELLSZ;
			if (cur >= bottom) {
				cur -= CELLSZ;
				x = 0;
			}
		}
		return;
	case 'L':		/* Insert line */
		while (x-- > 0) {
			p = cur - ((cur-top) % (COLS*CELLSZ));
			bcopy(p, p+(COLS*CELLSZ), lastl-p);
			blankline(p);
		}
		return;

	case 'M':		/* Delete line */
		while (x-- > 0) {
			p = cur - ((cur-top) % (COLS*CELLSZ));
			bcopy(p+(COLS*CELLSZ), p, lastl-p);
			blankline(lastl);
		}
		return;

	case '@@':		/* Insert character */
		while (x-- > 0) {
			y = cur-top;
			y = COLS*CELLSZ - (y % (COLS*CELLSZ));
			p = cur+y;
			bcopy(cur, cur+CELLSZ, (p-cur)-CELLSZ);
			*(ushort *)cur = BLANKW;
		}
		return;

	case 'P':		/* Delete character */
		while (x-- > 0) {
			y = cur-top;
			y = COLS*CELLSZ - (y % (COLS*CELLSZ));
			p = cur+y;
			bcopy(cur+CELLSZ, cur, (p-cur)-CELLSZ);
			p -= CELLSZ;
			*(ushort *)p = BLANKW;
		}
		return;

	case 'J':		/* Clear screen/eos */
		if (x == 1) {
			p = cur;
			while (p < bottom) {
				*(ushort *)p = BLANKW;
				p += CELLSZ;
			}
		} else {
			cls();
		}
		return;

	case 'H':		/* Position */
		cur = top + (x-1)*(COLS*CELLSZ) + (y-1)*CELLSZ;
		if (cur < top) {
			cur = top;
		} else if (cur >= bottom) {
			cur = lastl;
		}
		return;

	case 'K':		/* Clear to end of line */
		y = cur-top;
		y = COLS*CELLSZ - (y % (COLS*CELLSZ));
		p = cur+y;
		do {
			p -= CELLSZ;
			*(ushort *)p = BLANKW;
		} while (p > cur);
		return;

	default:
		/* Ignore */
		return;
	}
}

/*
 * do_multichar()
 *	Handle further characters in a multi-character sequence
 */
static int
do_multichar(int state, char c)
{
	static int x, y;

	switch (state) {
	case 1:		/* Escape has arrived */
		switch (c) {
		case 'P':	/* Cursor down a line */
			cur += (COLS*CELLSZ);
			if (cur >= bottom) {
				cur -= (COLS*CELLSZ);
			}
			return(0);
		case 'K':	/* Cursor left */
			cur -= CELLSZ;
			if (cur < top) {
				cur = top;
			}
			return(0);
		case 'H':	/* Cursor up */
			cur -= (COLS*CELLSZ);
			if (cur < top) {
				cur += (COLS*CELLSZ);
			}
			return(0);
		case 'M':	/* Cursor right */
			cur += CELLSZ;
			if (cur >= bottom) {
				cur -= CELLSZ;
			}
			return(0);
		case 'G':	/* Cursor home */
			cur = top;
			return(0);
		case '[':	/* Extended sequence */
			return(2);
		default:
			return(0);
		}

	case 2:		/* Seen Esc-[ */
		if (isdigit(c)) {
			x = c - '0';
			return(3);
		}
		sequence(1, 1, c);
		return(0);

	case 3:		/* Seen Esc-[<digit> */
		if (isdigit(c)) {
			x = x*10 + (c - '0');
			return(3);
		}
		if (c == ';') {
			y = 0;
			return(4);
		}
		sequence(x, 1, c);
		return(0);

	case 4:		/* Seen Esc-[<digits>; */
		if (isdigit(c)) {
			y = y*10 + (c - '0');
			return(4);
		}

		/*
		 * This wraps the sequence
		 */
		sequence(x, y, c);
		return(0);
	default:
		return(0);
	}
}

/*
 * write_string()
 *	Given a counted string, put the characters onto the screen
 */
void
write_string(char *s, uint cnt)
{
	char c;
	int x;
	static int state = 0;

	while (cnt--) {
		c = (*s++) & 0x7F;

		/*
		 * If we are inside a multi-character sequence,
		 * continue
		 */
		if (state > 0) {
			state = do_multichar(state, c);
			continue;
		}

		/*
		 * Printing characters are easy
		 */
		if ((c >= ' ') && (c < 0x7F)) {
			*cur++ = c;
			*cur++ = NORMAL;
			if (cur >= bottom) {	/* Scroll */
				scrollup();
				cur = lastl;
			}
			continue;
		}

		/*
		 * newline
		 */
		if (c == '\n') {
			/*
			 * Last line--just scroll
			 */
			if ((cur+COLS*CELLSZ) >= bottom) {
				scrollup();
				cur = lastl;
				continue;
			}

			/*
			 * Calculate address of start of next line
			 */
			x = cur-top;
			x = x + (COLS*CELLSZ - (x % (COLS*CELLSZ)));
			cur = top + x;
			continue;
		}

		/*
		 * carriage return
		 */
		if (c == '\r') {
			x = cur-top;
			x = x - (x % (COLS*CELLSZ));
			cur = top + x;
			continue;
		}

		/*
		 * \b--back up a space
		 */
		if (c == '\b') {
			if (cur > top) {
				cur -= CELLSZ;
			}
			continue;
		}

		/*
		 * \t--tab to next stop
		 */
		if (c == '\t') {
			/*
			 * Get current position
			 */
			x = cur-top;
			x %= (COLS*CELLSZ);

			/*
			 * Calculate steps to next tab stop
			 */
			x = (TABS*CELLSZ) - (x % (TABS*CELLSZ));

			/*
			 * Advance that many.  If we run off the end
			 * of the display, scroll and start at column
			 * zero.
			 */
			cur += x;
			if (cur >= bottom) {
				scrollup();
				cur = lastl;
			}
			continue;
		}

		/*
		 * Escape starts a multi-character sequence
		 */
		if (c == '\33') {
			state = 1;
			continue;
		}

		/*
		 * Ignore other control characters
		 */
	}
}
@


1.11
log
@Add support for -mono and -color
@
text
@d246 1
a246 1
			bcopy(cur, cur+1, (p-cur)-1);
d256 1
a256 1
			bcopy(cur+1, cur, (p-cur)-1);
@


1.10
log
@Add ESC-[ cursor motion, fix bug in state machine handling, and
remove needlessly abort()ive check.
@
text
@d18 1
d76 4
a79 4
	outportb(IDX, 0xE);
	outportb(DAT, (pos >> 8) & 0xFF);
	outportb(IDX, 0xF);
	outportb(DAT, pos & 0xFF);
d112 2
d116 1
a116 1
init_screen(void)
d121 13
d137 2
a138 2
	p = mmap((void *)DISPLAY, SCREENMEM,
		PROT_READ|PROT_WRITE, MAP_PHYS, 0, 0L);
@


1.9
log
@Get multiple active I/O screens working
@
text
@d171 10
d182 27
d296 4
d334 1
a359 3
#ifdef DEBUG
		abort();
#else
a360 1
#endif
@


1.8
log
@Add clear-screen interface, fix save/restore to always do their
operations WRT hardware
@
text
@d28 1
a39 1
	s->s_pos = cur-top;
@


1.7
log
@Allow multi-screen by adding an interface for the screen painting
algorithms to be applied to off-screen RAM.
@
text
@d11 1
d26 2
a27 2
	bcopy(s->s_img, top, SCREENMEM);
	cur = top + s->s_pos;
d38 1
a38 1
	bcopy(top, s->s_img, SCREENMEM);
d63 1
d82 16
d104 1
a104 6
	ulong bl, *u;

	bl = BLANK;
	for (u = (ulong *)top; u < (ulong *)bottom; ++u) {
		*u = bl;
	}
@


1.6
log
@Source reorg
@
text
@d8 3
d16 1
d19 46
d68 2
a69 2
static void
cursor()
d104 5
a108 1
	p = mmap((void *)DISPLAY, ROWS*COLS*CELLSZ,
d113 2
a114 3
	cur = top = p;
	bottom = p + (ROWS*COLS*CELLSZ);
	lastl = p + ((ROWS-1)*COLS*CELLSZ);
d237 1
a237 1
static
d318 1
a318 1
write_string(char *s, int cnt)
a430 6

	/*
	 * Only bother with cursor now that the whole string's
	 * on the screen.
	 */
	cursor();
@


1.5
log
@Add fansi extended commands--cleol, cleos
@
text
@d6 1
a6 1
#include <cons/cons.h>
@


1.4
log
@Add a buncha ANSI escape sequences.  MicroEmacs seems to work
pretty well with these provided.
@
text
@d125 1
a125 1
			y = y + (COLS*CELLSZ - (y % (COLS*CELLSZ)));
d128 1
a128 1
			cur[0] = ' ';
d135 1
a135 1
			y = y + (COLS*CELLSZ - (y % (COLS*CELLSZ)));
d139 1
a139 1
			p[0] = ' ';
d143 10
a152 2
	case 'J':		/* Clear (assume arg==2) */
		cls();
d162 10
@


1.3
log
@First pass at adding ANSI curosr functions
@
text
@d39 1
a39 1
	for (u = (ulong *)top; u < (ulong *)bottom; ++u)
d41 1
d76 1
a76 1
	for (u = (ulong *)lastl; u < (ulong *)bottom; ++u)
d78 16
d103 2
d106 38
a143 1
	case 'J':		/* Clear */
d173 27
a199 1
		if (c != '[') {
a201 1
		return(2);
@


1.2
log
@Add support for tabs
@
text
@d80 82
d170 1
d176 9
a229 9
		 * \f--clear screen, home cursor.
		 * It ain't ANSI, but it works....
		 */
		if (c == '\f') {
			cls();
			continue;
		}

		/*
d264 8
@


1.1
log
@Initial revision
@
text
@d157 28
@
