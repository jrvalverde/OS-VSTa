head	1.3;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.05.30.04.04.43;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.04.09.03.31.58;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.29.24;	author vandys;	state Exp;
branches;
next	;


desc
@Mouse interface library defs
@


1.3
log
@Add PS2, convert to syslog, convert to RS-232 server
@
text
@/*
 * libmouse.h
 *    Function prototypes for the mouse server API.
 *
 * Copyright (C) 1993 G.T.Nicol, all rights reserved.
 *
 * All the functions here return -1 on error, or 0 on success.
 */

#ifndef __VSTA_LIBMOUSE_H__
#define __VSTA_LIBMOUSE_H__

/*
 * Masks for the mouse buttons
 */
#define MOUSE_LEFT_BUTTON    (1 << 0)
#define MOUSE_RIGHT_BUTTON   (1 << 1)
#define MOUSE_MIDDLE_BUTTON  (1 << 2)

/*
 * Open/close the mouse device
 */
extern int mouse_connect(void);
extern int mouse_disconnect(void);

/*
 * Low level I/O routines
 */
extern int mouse_read(uchar *buttons, ushort *x, ushort *y, int *freq,
		      ushort *bx1, ushort *by1, ushort *bx2, ushort *by2);
extern int mouse_write(uchar *op, uchar *buttons, ushort *x, ushort *y, 
		       int *freq, ushort *bx1, ushort *by1, ushort *bx2, 
		       ushort *by2);

/*
 * Higher level routines
 */
extern int mouse_get_buttons(uchar *buttons);
extern int mouse_get_coordinates(ushort *x, ushort *y);
extern int mouse_get_bounds(ushort *x1, ushort *y1, ushort *x2, ushort *y2);
extern int mouse_get_update_freq(int *freq);

extern int mouse_set_coordinates(ushort x, ushort y);
extern int mouse_set_bounds(ushort x1, ushort y1, ushort x2, ushort y2);
extern int mouse_set_update_freq(int freq);

#endif /* __VSTA_LIBMOUSE_H__ */
@


1.2
log
@Clean up white space
@
text
@d16 2
a17 1
#define MOUSE_LEFT_BUTTON    (1 << 1)
a18 1
#define MOUSE_RIGHT_BUTTON   (1 << 3)
@


1.1
log
@Initial revision
@
text
@a47 2


@
