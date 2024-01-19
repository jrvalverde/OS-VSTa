head	1.3;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2;
locks; strict;
comment	@ * @;


1.3
date	94.05.30.04.04.43;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.01.13.06.33.14;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.29.24;	author vandys;	state Exp;
branches;
next	;


desc
@Defs for IBM serial mouse
@


1.3
log
@Add PS2, convert to syslog, convert to RS-232 server
@
text
@/* 
 * ibmrs232.h
 *    Polling driver for the IBM serial mouse.
 *
 * Copyright (C) 1993 by G.T.Nicol, all rights reserved.
 *
 */

#ifndef __MOUSE_IBM_SERIAL_H__
#define __MOUSE_IBM_SERIAL_H__

#define IBM_SERIAL_BUFSIZ 512

typedef enum {
  RS_MICROSOFT    = 0,
  RS_MOUSE_SYS_3  = 1,
  RS_MOUSE_SYS_5  = 2,
  RS_MM           = 3,
  RS_LOGITECH     = 4
} ibm_model_t;

extern int  ibm_serial_initialise(int argc, char **argv);
extern void ibm_serial_poller_entry_point(void);
extern void ibm_serial_coordinates(ushort x, ushort y);
extern void ibm_serial_bounds(ushort x1, ushort y1, ushort x2, ushort y2);
extern void ibm_serial_update_period(ushort period);

#endif /* __MOUSE_IBM_SERIAL_H__ */
@


1.2
log
@Bring up microsoft RS-232 mouse
@
text
@d12 2
d15 5
a19 6
  RS_UNKNOWN      = 0,
  RS_MICROSOFT    = 1,
  RS_MOUSE_SYS_3  = 2,
  RS_MOUSE_SYS_5  = 3,
  RS_MM           = 4,
  RS_LOGITECH     = 5,
a21 15
/*
 * I/O address of registers
 */
#define IBM_RS_LINEREG  (ibm_serial_iobase+0xB)  /* Format of RS-232 data  */
#define IBM_RS_LOWBAUD  (ibm_serial_iobase+0x8)  /* low parts of baud rate */
#define IBM_RS_HIBAUD   (ibm_serial_iobase+0x9)  /* low parts of baud rate */
#define IBM_RS_LINESTAT (ibm_serial_iobase+0xD)  /* Status of line         */
#define IBM_RS_DATA     (ibm_serial_iobase+0x8)  /* Read/write data here   */
#define IBM_RS_INTREG   (ibm_serial_iobase+0x9)  /* Interrupt control      */
#define IBM_RS_INTID    (ibm_serial_iobase+0xA)  /* Why "interrupted"      */
#define IBM_RS_MODEM    (ibm_serial_iobase+0xC)  /* Modem lines            */

#define IBM_RS_HIGH_PORT IBM_RS_MODEM
#define IBM_RS_LOW_PORT  IBM_RS_LOWBAUD

a26 20

/* interrupt identification register IBM_RS_INTID FIFO extension bits */
#define	IIR_FIFO_MASK	0xc0	/* set if FIFOs are enabled */

/* fifo control register (written to IBM_RS_INTID */
#define	FIFO_ENABLE	0x01
#define	FIFO_RCV_RST	0x02
#define	FIFO_XMT_RST	0x04
#define	FIFO_DMA_MODE	0x08
#define	FIFO_TRIGGER_1	0x00
#define	FIFO_TRIGGER_4	0x40
#define	FIFO_TRIGGER_8	0x80
#define	FIFO_TRIGGER_14	0xc0

/* IBM_RS_MODEM bits */
#define	MCR_LOOPBACK	0x10
#define	MCR_IENABLE	0x08
#define	MCR_DRS		0x04
#define	MCR_RTS		0x02
#define	MCR_DTR		0x01
@


1.1
log
@Initial revision
@
text
@d42 2
a43 1
#endif /* __MOUSE_IBM_SERIAL_H__ */
d45 16
d62 1
a62 7







@
