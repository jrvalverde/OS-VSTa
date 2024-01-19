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
date	93.11.16.02.51.47;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.07.02.22.03.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.05.04;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for HAT routines
@


1.3
log
@Source reorg
@
text
@#ifndef _MACHHAT_H
#define _MACHHAT_H
/*
 * hatvas.h
 *	Hardware-specific information pertaining to a vas
 */
#include <mach/vm.h>
#include <rmap.h>

struct hatvas {
	pte_t *h_vcr3;		/* CR3, in its raw form and as a vaddr */
	ulong h_cr3;
	struct rmap *h_map;	/* Map of vaddr's free */
	ulong h_l1segs;		/* Bit map of which L1 slots used */
};

#define H_L1SEGS (32)		/* # parts L1 PTE's broken into */

#endif /* _MACHHAT_H */
@


1.2
log
@Use a bitmap to mark which parts of the L1 PTEs have been filled
in.  This saves having to scan the whole L1 PTE page to find the
L2 PTEs which need to be freed.
@
text
@d8 1
a8 1
#include <lib/rmap.h>
@


1.1
log
@Initial revision
@
text
@d14 1
d16 2
@
