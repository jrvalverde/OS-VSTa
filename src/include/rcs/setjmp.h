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
date	93.03.08.18.47.48;	author vandys;	state Exp;
branches;
next	;


desc
@Wrapper for setjmp/longjmp
@


1.1
log
@Initial revision
@
text
@/*
 * setjmp.h
 *	Dummy wrapper to redirect to our machine-dependent one
 */
#include <mach/setjmp.h>
@
