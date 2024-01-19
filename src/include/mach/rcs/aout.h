head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.04.08.04.12.25;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.04.06.03.37.18;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.04.00;	author vandys;	state Exp;
branches;
next	;


desc
@A simple definition of the shape of an a.out header
@


1.3
log
@Remove unneded types.h
@
text
@#ifndef _AOUT_H
#define _AOUT_H
/*
 * aout.h
 *	A very abbreviated notion of what an a.out header looks like
 */
struct aout {
unsigned long
	a_info,		/* Random stuff, already checked */
	a_text,		/* length of text in bytes */
	a_data,		/* length of data in bytes */
	a_bss,		/* length of bss, in bytes */
	a_syms,		/* symbol stuff, ignore */
	a_entry,	/* entry point */
	a_trsize,	/* relocation stuff, ignore */
	a_drsize;	/*  ...ditto */
};

#endif /* _AOUT_H */
@


1.2
log
@Use only native data types, so that dbsym.c can use these
files without involving itself in VSTa's types.h
@
text
@a6 2
#include <sys/types.h>

@


1.1
log
@Initial revision
@
text
@d10 1
a10 1
ulong
@
