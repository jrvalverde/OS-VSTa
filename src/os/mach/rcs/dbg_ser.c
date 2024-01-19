head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.2
date	93.08.24.00.41.03;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.07.01;	author vandys;	state Exp;
branches;
next	;


desc
@Spin-oriented RS232 routines for kernel printf()'s and debugger.
@


1.2
log
@Add kdb support for console, plus hooks so you can still do serial
if needed.
@
text
@/*
 * dbg_ser.c
 *	Routines to do a spin-oriented interface to a serial port
 */
#include <mach/param.h>

#ifdef SERIAL

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
#define DATA (IOBASE+0x8)	/* Read/write data here */
#define INTREG (IOBASE+0x9)	/* Interrupt control */
#define INTID (IOBASE+0xA)	/* Why "interrupted" */
#define MODEM (IOBASE+0xC)	/* Modem lines */

/*
 * init_cons()
 *	Initialize to 9600 baud on com port
 */
void
init_cons(void)
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
void
cons_putc(int c)
{
	while ((inportb(LINESTAT) & 0x20) == 0)
		;
	outportb(DATA, c & 0x7F);
}

#ifdef KDB
/*
 * cons_getc()
 *	Busy-wait and return next character
 */
int
cons_getc(void)
{
	while ((inportb(LINESTAT) & 1) == 0)
		;
	return(inportb(DATA) & 0x7F);
}
#endif /* KDB */

#endif /* SERIAL */
@


1.1
log
@Initial revision
@
text
@d2 1
a2 1
 * rs232.c
d5 1
d7 2
d28 1
a28 1
 * rs232_init()
d32 1
a32 1
rs232_init(void)
d45 1
a45 1
rs232_putc(int c)
d52 1
d54 1
a54 1
 * rs232_getc()
d57 2
a58 1
rs232_getc(void)
d64 3
@
