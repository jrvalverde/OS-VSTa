head	1.6;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.5
	V1_1:1.5
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.6
date	94.11.05.10.06.03;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.10.02.22.01.09;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.24.00.41.03;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.23.22.42.51;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.05.00.38.04;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.04.11;	author vandys;	state Exp;
branches;
next	;


desc
@Wrapper around call to the dbg/ routines
@


1.6
log
@Add counters to track CPU hogs
@
text
@/*
 * dbg.c
 *	A simple/simplistic debug interface
 *
 * Hard-wired to talk out either console or display.  If KDB isn't
 * configured, only the parts needed to display kernel printf()'s
 * are compiled in.
 */

extern void cons_putc(int);
static int col = 0;		/* When to wrap */

#ifdef KDB
extern int cons_getc(void);
static char buf[80];		/* Typing buffer */
static int prlines = 0;		/* # lines since last paused */
#endif

/*
 * more()
 *	Sorta.  Pause if we've scrolled too much text
 */
static void
more(void)
{
#ifdef KDB
	if (++prlines < 23)
		return;
	(void)cons_getc();
	prlines = 0;
#endif
}

/*
 * putchar()
 *	Write a character to the debugger port
 *
 * Brain damage as my serial terminal doesn't wrap columns.
 */
void
putchar(int c)
{
	if (c == '\n') {
		col = 0;
		cons_putc('\r');
		cons_putc('\n');
		more();
	} else {
		if (++col >= 78) {
			cons_putc('\r'); cons_putc('\n');
			more();
			col = 1;
		}
		cons_putc(c);
	}
}

#ifdef KDB
/*
 * getchar()
 *	Get a character from the debugger port
 */
getchar(void)
{
	char c;

	prlines = 0;
	c = cons_getc() & 0x7F;
	if (c == '\r')
		c = '\n';
	return(c);
}

/*
 * gets()
 *	A spin-oriented "get line" routine
 */
void
gets(char *p)
{
	char c;
	char *start = p;

	putchar('>');
	for (;;) {
		c = getchar();
		if (c == '\b') {
			if (p > start) {
				printf("\b \b");
				p -= 1;
			}
		} else if (c == '') {
			p = start;
			printf("\\\n");
		} else {
			putchar(c);
			if (c == '\n')
				break;
			*p++ = c;
		}
	}
	*p = '\0';
}

/*
 * dbg_enter()
 *	Basic interface for debugging
 */
void
dbg_enter(void)
{
	int was_on;
	extern void dbg_main(void);

	was_on = cli();
	printf("[Kernel debugger]\n");
	dbg_main();
	if (was_on) {
		sti();
	}
}
#endif /* KDB */
@


1.5
log
@Add auto-more so text doesn't scroll off & become unreadable
@
text
@d112 1
d115 1
d118 3
@


1.4
log
@Add kdb support for console, plus hooks so you can still do serial
if needed.
@
text
@d16 1
d20 15
d46 2
d51 1
d54 1
a55 1
	cons_putc(c);
d67 1
@


1.3
log
@Implement KDB
@
text
@d5 3
a7 2
 * Hard-wired to talk out the COM1 serial port at 9600 baud.  Does
 * not use interrupts.
d9 4
d14 1
a15 1
static int dbg_init = 0;
a16 46
static int col = 0;		/* When to wrap */

/*
 * 1 for COM1, 0 for COM2 (bleh)
 */
#define COM (1)

/*
 * I/O address of registers
 */
#define IOBASE (0x2F0 + COM*0x100)	/* Base of registers */
#define LINEREG (IOBASE+0xB)	/* Format of RS-232 data */
#define LOWBAUD (IOBASE+0x8)	/* low/high parts of baud rate */
#define HIBAUD (IOBASE+0x9)
#define LINESTAT (IOBASE+0xD)	/* Status of line */
#define  RXRDY (1)		/* Character assembled */
#define  TXRDY (0x20)		/* Transmitter ready for next char */
#define DATA (IOBASE+0x8)	/* Read/write data here */
#define INTREG (IOBASE+0x9)	/* Interrupt control */
#define INTID (IOBASE+0xA)	/* Why "interrupted" */
#define MODEM (IOBASE+0xC)	/* Modem lines */

/*
 * rs232_init()
 *	Initialize to 9600 baud on com port
 */
static void
rs232_init(void)
{
	outportb(LINEREG, 0x80);	/* 9600 baud */
	outportb(HIBAUD, 0);
	outportb(LOWBAUD, 0x0C);
	outportb(LINEREG, 3);		/* 8 bits, one stop */
}

/*
 * rs232_putc()
 *	Busy-wait and then send a character
 */
static void
rs232_putc(int c)
{
	while ((inportb(LINESTAT) & 0x20) == 0)
		;
	outportb(DATA, c & 0x7F);
}
d29 1
a29 1
		rs232_putc('\r');
d32 1
a32 1
			rs232_putc('\r'); rs232_putc('\n');
d36 1
a36 1
	rs232_putc(c);
a40 12
 * rs232_getc()
 *	Busy-wait and return next character
 */
static
rs232_getc(void)
{
	while ((inportb(LINESTAT) & 1) == 0)
		;
	return(inportb(DATA) & 0x7F);
}

/*
d48 1
a48 1
	c = rs232_getc() & 0x7F;
d92 1
a92 1
	extern void dbg_main();
a93 4
	if (!dbg_init) {
		rs232_init();
		dbg_init = 1;
	}
@


1.2
log
@Add line wrap in software--my terminal doesn't have it.  Duh.
@
text
@d8 1
d11 1
a59 12
 * rs232_getc()
 *	Busy-wait and return next character
 */
static
rs232_getc(void)
{
	while ((inportb(LINESTAT) & 1) == 0)
		;
	return(inportb(DATA) & 0x7F);
}

/*
d80 13
d154 1
@


1.1
log
@Initial revision
@
text
@d10 1
d72 2
d78 2
a79 1
	if (c == '\n')
d81 6
@
