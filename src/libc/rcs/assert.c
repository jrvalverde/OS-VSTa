head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.3
date	93.08.29.22.53.49;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.05.03.21.30.06;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.56.15;	author vandys;	state Exp;
branches;
next	;


desc
@Assertion failure support routines
@


1.3
log
@Add ANSI dec'ls for assfail()
@
text
@/*
 * assert.c
 *	Supporting routines for the ASSERT/ASSERT_DEBUG macros
 */
#include <stdio.h>

/*
 * assfail()
 *	Called from ASSERT-type macros on failure
 */
void
assfail(const char *msg, const char *file, int line)
{
	fprintf(stderr, "Assertion failed in file %s, line %d\n",
		file, line);
	fprintf(stderr, "Fatal error: %s\n", msg);
	abort();
}
@


1.2
log
@Get rid of special kernel support
@
text
@d12 1
a12 1
assfail(msg, file, line)
@


1.1
log
@Initial revision
@
text
@a4 1
#ifndef KERNEL
a5 1
#endif
a13 4
#ifdef KERNEL
	printf("Assertion failed in file %s, line %d\n", file, line);
	panic(msg);
#else
a17 1
#endif
@
