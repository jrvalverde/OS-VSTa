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
date	93.01.29.16.06.32;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions of i386/ISA NVRAM configuration area
@


1.1
log
@Initial revision
@
text
@
#ifndef _NVRAM_H
#define _NVRAM_H
/*
 * nvram.h
 *	Generic constants for talking to NVRAM information
 *
 * Device-specific portions of the information reside in the
 * per-device include files.
 */
#define RTCSEL 0x70		/* Port to select */
#define RTCDATA 0x71		/* Data port */

#endif /* _NVRAM_H */
@
