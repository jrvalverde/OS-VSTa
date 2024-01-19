head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.2
date	94.02.01.23.14.51;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.32.48;	author vandys;	state Exp;
branches;
next	;


desc
@Programmable interrupt timer
@


1.2
log
@Fix commenting
@
text
@#ifndef _MACHPIT_H
#define _MACHPIT_H
/*
 * pit.h
 *	Constants relating to the 8254 (or 8253) programmable interval timers
 */
 
/*
 * Port address of the control port and timer channels
 */
#define PIT_CTRL 0x43
#define PIT_CH0 0x40
#define PIT_CH1 0x41
#define PIT_CH2 0x42

/*
 * Command to set square wave clock mode
 */
#define CMD_SQR_WAVE 0x36

/*
 *The internal tick rate in ticks per second
 */
#define PIT_TICK 1193180

#endif /* _MACHPIT_H */
@


1.1
log
@Initial revision
@
text
@d8 3
a10 1
/* Port address of the control port and timer channels */
d16 3
a18 1
/* Command to set square wave clock mode */
d21 3
a23 1
/* The internal tick rate in ticks per second */
@
