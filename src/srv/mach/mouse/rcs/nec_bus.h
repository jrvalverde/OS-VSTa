head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.2
date	94.04.09.03.32.09;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.30.12;	author vandys;	state Exp;
branches;
next	;


desc
@NEC bus mouse defs
@


1.2
log
@Clean up white space
@
text
@/* 
 * nec_bus.h
 *    Combined polling/interrupt driven driver for the NEC bus mouse.
 *
 * Copyright (C) 1993 by G.T.Nicol, all rights reserved.
 *
 * This driver does not currently support high-resolution mode.
 */

#ifndef __MOUSE_NEC_BUS_H__
#define __MOUSE_NEC_BUS_H__

#define PC98_MOUSE_TIMER_PORT     0xbfdb
#define PC98_MOUSE_RPORT_A        0x7fd9
#define PC98_MOUSE_RPORT_B        0x7fdb
#define PC98_MOUSE_RPORT_C        0x7fdd
#define PC98_MOUSE_WPORT_C        0x7fdd
#define PC98_MOUSE_MODE_PORT      0x7fdf
#define PC98_MOUSE_LOW_PORT       PC98_MOUSE_RPORT_A
#define PC98_MOUSE_HIGH_PORT      PC98_MOUSE_MODE_PORT

extern int  pc98_bus_initialise(int argc, char **argv);
extern void pc98_bus_interrupt(void);
extern void pc98_bus_poller_entry_point(void);
extern void pc98_bus_coordinates(ushort x, ushort y);
extern void pc98_bus_bounds(ushort x1, ushort y1, ushort x2, ushort y2);
extern void pc98_bus_update_period(ushort period);

#endif /* __MOUSE_BUS_NEC_H__ */
@


1.1
log
@Initial revision
@
text
@a29 9









@
