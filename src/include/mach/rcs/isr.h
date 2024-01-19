head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.2
date	93.02.12.19.44.43;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.05.39;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for ISR handling
@


1.2
log
@Add defs for slave PIC
@
text
@#ifndef _MACHISR_H
#define _MACHISR_H
/*
 * isr.h
 *	Constants relating to the ISA interrupt architecture
 */
#define MAX_IRQ 16	/* Max different IRQ levels */
#define SLAVE_IRQ 8	/* 0..7 are master, 8..15 are slave */
#define MASTER_SLAVE 2	/* IRQ where slave hooks to master */

#endif /* _MACHISR_H */
@


1.1
log
@Initial revision
@
text
@d8 2
@
