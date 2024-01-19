head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2;
locks; strict;
comment	@ * @;


1.2
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.08.58.55;	author vandys;	state Exp;
branches;
next	;


desc
@Block free-list definitions
@


1.2
log
@Source reorg
@
text
@#ifndef ALLOC_H
#define ALLOC_H
/*
 * alloc.h
 *	Definitions for managing the free block list
 */
#include "vstafs.h"

/*
 * We always keep the free list blocks in-core, and just flush out
 * the appropriate ones as needed.  This data structure tabulates
 * the in-core copy.
 */
struct freelist {
	daddr_t fr_this;	/* Sector # for this free list block */
	struct free fr_free;	/* Image of free list block on disk */
	struct freelist		/* Core version of f_free.f_next */
		*fr_next;
};

/*
 * Externally-usable routines
 */
extern void init_block(void);
extern daddr_t alloc_block(uint);
extern void free_block(daddr_t, uint);
extern ulong take_block(daddr_t, ulong);

#endif /* ALLOC_H */
@


1.1
log
@Initial revision
@
text
@d7 1
a7 1
#include <vstafs/vstafs.h>
@
