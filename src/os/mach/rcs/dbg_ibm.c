head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3;
locks; strict;
comment	@ * @;


1.3
date	93.09.18.18.49.36;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.24.01.18.09;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.24.00.36.54;	author vandys;	state Exp;
branches;
next	;


desc
@IBM console debug driver
@


1.3
log
@More convenient to have debugger start at bottom
@
text
@/*
 * dbg_ibm.c 
 *	IBM Console interface routines for kernel debugger
 *
 * This code contributed by G.T.Nicol.
 */
#include <mach/param.h>

#ifndef SERIAL

#define COLOR			/* Else MGA text locations */
#define DBG_MAX_COL  80		/* Maximum number of columns */
#define DBG_MAX_ROW  25		/* Maximum number of rows */
#define DBG_RAM_SIZE (80*25)	/* Size of screen, in short's */

/*
 * Pick base of video RAM and control ports depending on which
 * type of interface we're using.
 */
#ifdef COLOR
#define GDC_REG_PORT 0x3d4
#define GDC_VAL_PORT 0x3d5
#define TVRAM 0xb8000    
#else
#define GDC_REG_PORT 0x3b4
#define GDC_VAL_PORT 0x3b5
#define TVRAM 0xb0000    
#endif

/*
 * White character attribute--works for MGA and CGA
 */
#define WHITE 0x07

static unsigned short *dbg_tvram = (unsigned short *) TVRAM;
static unsigned char dbg_current_col = 0;
static unsigned char dbg_current_row = 24; /* start debugger at bottom */

/*
 * init_cons()
 *	Hook for any early setup
 */
void
init_cons(void)
{
	/* Nothing */ ;
}

/*
 * dbg_set_cursor_pos()
 *	Set cursor position based on current absolute screen offset
 */
static void
dbg_set_cursor_pos(void)
{
	unsigned short gdc_pos;

	gdc_pos = (dbg_current_row * DBG_MAX_COL) + dbg_current_col;
	outportb(GDC_REG_PORT, 0xe);
	outportb(GDC_VAL_PORT, (gdc_pos >> 8) & 0xFF);
	outportb(GDC_REG_PORT, 0xf);
	outportb(GDC_VAL_PORT, gdc_pos & 0xff);
}

/*
 * dbg_scroll_up()
 *	Scroll screen up one line
 */
static void
dbg_scroll_up(void)
{
	unsigned int loop;

	for (loop = 0; loop < (DBG_MAX_ROW - 1) * DBG_MAX_COL; loop++) {
		dbg_tvram[loop] = dbg_tvram[loop + DBG_MAX_COL];
	}
	for (loop = (DBG_RAM_SIZE)-DBG_MAX_COL; loop < DBG_RAM_SIZE; loop++) {
		dbg_tvram[loop] = (WHITE<<8)|' ';
	}
}

/*
 * PUT()
 *	Write character at current screen location
 */
#define PUT(c) (dbg_tvram[(dbg_current_row * DBG_MAX_COL) + \
	dbg_current_col] = (WHITE << 8) | (c))

/*
 * cons_putc()
 *	Place a character on next screen position
 */
void
cons_putc(int c)
{
	switch (c) {
	case '\t':
		while (dbg_current_row % 8)
			cons_putc(' ');
		break;
	case '\r':
		dbg_current_col = 0;
		break;
	case '\n':
		dbg_current_row += 1;
		if (dbg_current_row >= DBG_MAX_ROW) {
			dbg_scroll_up();
			dbg_current_row -= 1;
		}
		break;
	case '\b':
		if (dbg_current_col > 0) {
			dbg_current_col -= 1;
			PUT(' ');
		}
		break;
	default:
		PUT(c);
		dbg_current_col += 1;
		if (dbg_current_col >= DBG_MAX_COL) {
			dbg_current_col = 0;
			dbg_current_row += 1;
			if (dbg_current_row >= DBG_MAX_ROW) {
				dbg_scroll_up();
				dbg_current_row -= 1;
			}
		}
	};
	dbg_set_cursor_pos();
}

#ifdef KDB
/*
 * Ports for PC keyboard
 */
#define KBD_CTL 0x61
#define KBD_DATA 0x60
#define KBD_STATUS 0x64

/* These two come from kdb/isr.c */
static char key_map[] = {
  0,033,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
'q','w','e','r','t','y','u','i','o','p','[',']',015,0x80,
'a','s','d','f','g','h','j','k','l',';',047,0140,0x80,
0134,'z','x','c','v','b','n','m',',','.','/',0x80,
'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,'0',0177
};
static char shift_map[] = {
  0,033,'!','@@','#','$','%','^','&','*','(',')','_','+','\b','\t',
'Q','W','E','R','T','Y','U','I','O','P','{','}',015,0x80,
'A','S','D','F','G','H','J','K','L',':',042,'~',0x80,
'|','Z','X','C','V','B','N','M','<','>','?',0x80,
'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
0x80,0x80,0x80,0x80,'7','8','9',0x80,'4','5','6',0x80,
'1','2','3','0',177
};

/*
 * dbg_getc()
 *	Get next character typed on PC keyboard
 */
int
cons_getc(void)
{
	unsigned char c;
	unsigned char outch;
	static int shift_pressed = 0,
		ctrl_pressed = 0,
		caps_pressed = 0;

again:
	/*
	 * Read keyboard controller, toggle enable
	 */
	c = inportb(KBD_CTL);
	outportb(KBD_CTL, c & ~0x80);
	outportb(KBD_CTL, c | 0x80);
	outportb(KBD_CTL, c & ~0x80);

	/*
	 * Wait for it to tell us it has data
	 */
	while (((c = inportb(KBD_STATUS)) & 0x01) == 0) {
		;
	}

	/*
	 * Read the data.  Handle nonsense with shift, control, etc.
	 */
	c = inportb(KBD_DATA);
	switch (c) {
	case 0x36:
	case 0x2a:
		shift_pressed = 1;
		goto again;
	case 0x3a:
		caps_pressed = 1;
		goto again;
	case 0x1d:
		ctrl_pressed = 1;
		goto again;
	case 0xb6:
	case 0xaa:
		shift_pressed = 0;
		goto again;
	case 0xba:
		caps_pressed = 0;
		goto again;
	case 0x9d:
		ctrl_pressed = 0;
		goto again;
	/*
	 * Ignore unrecognized keys--usually arrow and such
	 */
	default:
		if (c & 0x80 || c > 0x39) {
			goto again;
		}
	}

	/*
	 * Strip high bit, look up in our map
	 */
	c &= 127;
	outch = shift_pressed ? shift_map[c] : key_map[c];

	/*
	 * If CAPS LOCK, convert to upper case
	 */
	if (caps_pressed) {
		if (outch >= 'A' && outch <= 'Z') {
			outch += 'a' - 'A';
		} else {
			if (outch >= 'a' && outch <= 'z')
				outch -= 'a' - 'A';
		}
	}

	/*
	 * If CTRL pressed, convert to a control character
	 */
	if (ctrl_pressed) {
		outch = key_map[c];
		outch &= 037;
	}

	return (outch);
}
#endif /* KDB */

#endif /* !SERIAL */
@


1.2
log
@Expunge NEC stuff; Nick will handle
@
text
@d37 1
a37 1
static unsigned char dbg_current_row = 10; /* start debugger at row 10 */
@


1.1
log
@Initial revision
@
text
@a8 1
#ifndef NEC
a253 2

#endif /* !NEC */
@
