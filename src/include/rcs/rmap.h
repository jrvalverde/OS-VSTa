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
date	93.01.29.15.57.54;	author vandys;	state Exp;
branches;
next	;


desc
@Resource map object definitions
@


1.1
log
@Initial revision
@
text
@#ifndef _RMAP_H
#define _RMAP_H
/*
 * rmap.h
 *	A resource map
 *
 * A resource map is represented as an open array.  You get an array
 * of X of them, and then you pass it and the count to rmap_init().
 */
#include <sys/types.h>

struct rmap {
	uint r_off;
	uint r_size;
};

/*
 * Routines you can call
 */
extern void rmap_init(struct rmap *, uint);
extern uint rmap_alloc(struct rmap *, uint);
extern void rmap_free(struct rmap *, uint, uint);

#endif /* _RMAP_H */
@
