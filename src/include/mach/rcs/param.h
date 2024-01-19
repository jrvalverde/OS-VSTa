head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.4
date	93.12.27.22.37.49;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.24.00.41.03;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.23.19.35.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.06.43;	author vandys;	state Exp;
branches;
next	;


desc
@Machine-dependent parameters
@


1.4
log
@Use new timer support to pick a better HZ (and an integer one,
so our timing will actually stay right).
@
text
@/*
 * param.h
 *	Global parameters
 */
#ifndef _MACHPARAM_H
#define _MACHPARAM_H

/*
 * Values and macros relating to hardware pages
 */
#define NBPG (4096)	/* # bytes in a page */
#define PGSHIFT (12)	/* LOG2(NBPG) */
#define ptob(x) ((ulong)(x) << PGSHIFT)
#define btop(x) ((ulong)(x) >> PGSHIFT)
#define btorp(x) (((ulong)(x) + (NBPG-1)) >> PGSHIFT)

/*
 * How often our clock "ticks"
 */
#define HZ (20)

/*
 * Which console device to use for printf() and such
 */
#undef SERIAL		/* Define for COM1, else use screen */

#endif /* _MACHPARAM_H */
@


1.3
log
@Add kdb support for console, plus hooks so you can still do serial
if needed.
@
text
@d20 1
a20 1
#define HZ (18)
@


1.2
log
@Change HZ to timer interval in PC.  Actually, should be 18.2.
Think about tweaking something to keep clock accurate.
@
text
@d22 5
@


1.1
log
@Initial revision
@
text
@d20 1
a20 1
#define HZ (100)
@
