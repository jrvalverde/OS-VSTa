head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.01.29.16.05.10;	author vandys;	state Exp;
branches;
next	;


desc
@Definition of i386/ISA Interrupt Control Unit
@


1.1
log
@Initial revision
@
text
@#ifndef _ICU_H
#define _ICU_H
/*
 * icu.h
 *	i386/ISA Interrupt Control Unit definitions
 *
 * The i386 has a pair of them, chained together.
 */

/* Base addresses of the two units */
#define ICU0 (0x20)
#define ICU1 (0xA0)

/* Clear an interrupt and allow new ones to arrive */
#define EOI() {outportb(ICU0, 0x20); outportb(ICU1, 0x20);}

/* Set interrupt mask */
#define SETMASK(mask) {outportb(ICU0+1, mask & 0xFF); \
	outportb(ICU1+1, (mask >> 8) & 0xFF); }

#endif /* _ICU_H */
@
