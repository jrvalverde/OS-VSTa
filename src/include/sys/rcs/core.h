head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.4
date	93.03.30.01.08.51;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.10.18.09.21;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.04.19.40.13;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.13.58;	author vandys;	state Exp;
branches;
next	;


desc
@Per-physical-page data structures
@


1.4
log
@Add bit to flag when a page is out for use from the general
memory pool.
@
text
@#ifndef _CORE_H
#define _CORE_H
/*
 * core.h
 *	Data structures organizing core
 */
#include <sys/types.h>

/*
 * Per-physical-page information
 */
struct core {
	ushort c_flags;		/* Flags */
	ushort c_psidx;		/* Index into pset */
	union {
	struct pset *_c_pset;	/* Pset page is used under */
	ulong _c_long;		/* Word of storage when C_SYS */
	struct core *_c_free;	/* Free list link when free */
	} _c_u;
#define c_pset _c_u._c_pset
#define c_long _c_u._c_long
#define c_free _c_u._c_free
};

/*
 * Bits in c_flags
 */
#define C_BAD 1		/* Hardware error on page */
#define C_SYS 2		/* Page wired down for kernel use */
#define C_WIRED 4	/* Wired for physical I/O */
#define C_ALLOC 8	/* Allocated from free list */

#endif /* _CORE_H */
@


1.3
log
@Add page wiring support bit
@
text
@d31 1
@


1.2
log
@VM system rewrite, move attach lists to perpage to allow ref count
of 0 to be allowed for valid slot.
@
text
@d30 1
@


1.1
log
@Initial revision
@
text
@a9 9
 * For enumerating current mappings of a physical page
 */
struct atl {
	struct pview *a_pview;
	uint a_idx;
	struct atl *a_next;
};

/*
d13 2
a14 2
	uchar c_flags;		/* Flags */
	uchar c_pad[3];		/* Padding */
d16 7
a22 5
		struct atl *_c_atl;	/* List of mappings */
		ulong _c_long;		/* Word of storage when C_SYS */
	} c_u;
#define c_atl c_u._c_atl
#define c_long c_u._c_long
a29 8

#ifdef KERNEL
/*
 * Routines for manipulating attach lists
 */
void atl_add(int, struct pview *, uint);
void atl_del(int, struct pview *, uint);
#endif
@
