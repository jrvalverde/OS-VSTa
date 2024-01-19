head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.10.02.20.21.24;	author vandys;	state Exp;
branches;
next	;


desc
@Test which chars are isspace()
@


1.1
log
@Initial revision
@
text
@#include <ctype.h>

main()
{
	unsigned int x;

	for (x = 0; x < 128; ++x) {
		if (isspace(x)) printf(" %02x", x);
	}
	return(0);
}
@
