head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.3
date	94.02.28.22.02.10;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.05.14.02.33;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.16.34;	author vandys;	state Exp;
branches;
next	;


desc
@Well Known Addresses for some servers.  Only needed during boot.
@


1.3
log
@Obsolete PORT_KBD
@
text
@#ifndef _PORTS_H
#define _PORTS_H
/*
 * ports.h
 *	Various global constants for port numbers
 */
#include <sys/types.h>

#define PORT_NAMER (port_name)1		/* Name to port # mapper */
#define PORT_TIMER (port_name)2		/* Central time service */
#define PORT_SWAP (port_name)3		/* Swap manager */
#define PORT_CONS (port_name)4		/* Console output */
/* 5 was keyboard port, now part of PORT_CONS */
#define PORT_ENV (port_name)6		/* Environment server */

#endif /* _PORTS_H */
@


1.2
log
@Add environment port address
@
text
@d13 1
a13 1
#define PORT_KBD (port_name)5		/* Console keyboard */
@


1.1
log
@Initial revision
@
text
@d7 1
a7 1
#include <sys/msg.h>
d14 1
@
