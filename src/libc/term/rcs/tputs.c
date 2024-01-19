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
date	93.03.19.00.55.37;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of string printing
@


1.1
log
@Initial revision
@
text
@/*
 * tputs.c
 *	Put a termcap-style string
 *
 * Based on technology:
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * In a world of LANs, X terminals, and memory-mapped displays, the
 * old garbage for estimating delays doesn't seem worth the memory
 * and CPU time.  This version merely ignores such things.
 */
#include <ctype.h>

/*
 * Put the character string cp out, with padding.
 * The number of affected lines is affcnt, and the routine
 * used to output one character is outc.
 */
tputs(cp, affcnt, outc)
	register char *cp;
	int affcnt;
	int (*outc)();
{
	if (cp == 0)
		return;

	/*
	 * Skip delay count
	 */
	while (isdigit(*cp)) {
		++cp;
	}
	if (*cp == '.') {
		cp++;
		while (isdigit(*cp)) {
			++cp;
		}
	}

	/*
	 * If the delay is followed by a `*', then
	 * skip multiplier.
	 */
	if (*cp == '*') {
		cp++;
	}

	/*
	 * The guts of the string.
	 */
	while (*cp) {
		(*outc)(*cp++);
	}
}
@
