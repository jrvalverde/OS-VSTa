head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.1
date	94.03.01.17.23.27;	author vandys;	state Exp;
branches;
next	;


desc
@Console server definitions
@


1.1
log
@Initial revision
@
text
@#ifndef _CONS_H
#define _CONS_H
/*
 * cons.h
 *	Wrapper to #include the right "real" file
 */
#ifdef IBM_CONSOLE
#include <mach/con_ibm.h>
#endif

#ifdef NEC_CONSOLE
#include <mach/con_nec.h>
#endif

#endif /* _CONS_H */
@
