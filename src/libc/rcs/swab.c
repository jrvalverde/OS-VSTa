head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2;
locks; strict;
comment	@ * @;


1.2
date	94.04.10.23.50.48;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.04.10.23.40.37;	author vandys;	state Exp;
branches;
next	;


desc
@Swap bytes
@


1.2
log
@Missing #include
@
text
@/*
 * swab()
 *	Swap pairs of bytes in a string
 */
#include <string.h>

void
swab(const char *src, char *dest, size_t len)
{
	for ( ; len > 1; len -= 2) {
		dest[1] = src[0];
		dest[0] = src[1];
		src += 2;
		dest += 2;
	}
}
@


1.1
log
@Initial revision
@
text
@d5 2
d8 1
a8 1
swap(const char *src, char *dest, size_t len)
@
