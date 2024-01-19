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
date	93.08.03.19.35.26;	author vandys;	state Exp;
branches;
next	;


desc
@A dumb little assertion macro
@


1.1
log
@Initial revision
@
text
@#ifndef _ASSERT_H
#define _ASSERT_H
/*
 * assert.h
 *	User version
 *
 * There's also one in sys/, used by the kernel
 */
extern void abort(void);

#define assert(cond) {if (!(cond)) abort();}

#endif /* _ASSERT_H */
@
