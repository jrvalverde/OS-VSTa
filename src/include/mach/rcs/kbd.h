head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.2
date	94.02.28.19.14.49;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.49.37;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for keyboard hardware and messages
@


1.2
log
@Change so sharable between two console servers.  Add F1/F10 so
multi-screen can recognize them.
@
text
@#ifndef _KEYBD_H
#define _KEYBD_H
/*
 * kbd.h
 *	Declarations for PC/AT keyboard controller
 */

#define KEYBD_MAXBUF (1024)	/* # bytes buffered from keyboard */

/*
 * I/O ports used
 */
#define KEYBD_LOW KEYBD_DATA

#define KEYBD_DATA 0x60
#define KEYBD_CTL 0x61
#define KEYBD_STATUS 0x64

#define KEYBD_HIGH KEYBD_STATUS

/*
 * Bit in KEYBD_CTL for strobing enable
 */
#define KEYBD_ENABLE 0x80

/*
 * Bits in KEYBD_STATUS
 */
#define KEYBD_BUSY 0x2
#define KEYBD_WRITE 0xD1

/*
 * Command for KEYBD_DATA to turn on high addresses
 */
#define KEYBD_ENAB20 0xDF

/*
 * Interrupt vector
 */
#define KEYBD_IRQ 1	/* Hardware IRQ1==interrupt vector 9 */

/*
 * Function key scan codes
 */
#define F1 (59)
#define F10 (68)

#ifdef KBD
#include <sys/types.h>

/*
 * Structure for per-connection operations
 */
struct file {
	int f_sender;	/* Sender of current operation */
	uint f_gen;	/* Generation of access */
	uint f_flags;	/* User access bits */
	uint f_count;	/* # bytes wanted for current op */
};
#endif

#endif /* _KEYBD_H */
@


1.1
log
@Initial revision
@
text
@a6 1
#include <sys/types.h>
d42 8
d60 1
@
