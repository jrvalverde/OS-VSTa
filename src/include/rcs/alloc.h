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
date	93.01.29.15.55.55;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for memory allocator interfaces, mostly now handled
via files in include/.
@


1.1
log
@Initial revision
@
text
@#ifndef _ALLOC_H
#define _ALLOC_H
/*
 * alloc.h
 *	Some common defs for allocation interfaces
 */
#include <sys/types.h>

#define alloca(s) __builtin_alloca(s)

extern void *malloc(uint), *realloc(void *, uint);
extern void free(void *);

#endif /* _ALLOC_H */
@
