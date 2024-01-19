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
date	94.03.08.23.21.49;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.54.06;	author vandys;	state Exp;
branches;
next	;


desc
@In-kernel random number generator.  Really.  Used in the scheduler.
@


1.2
log
@Allow state in random number generator to advance
@
text
@/*
 * rand.c
 *	Simple random number generator
 */
#include <sys/types.h>

static ulong state = 0L;

ulong
random(void)
{
	return((state = (state * 1103515245L + 12345L)) & 0x7fffffffL);
}

void
srandom(ulong l)
{
	state = l;
}
@


1.1
log
@Initial revision
@
text
@d12 1
a12 1
	return((state * 1103515245L + 12345L) & 0x7fffffffL);
@
