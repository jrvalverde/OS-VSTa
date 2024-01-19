head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.4
date	94.05.30.21.32.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.03.04.02.02.58;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.15.14.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.17.04;	author vandys;	state Exp;
branches;
next	;


desc
@Page View data structure
@


1.4
log
@Move struct refs to central place (works better)
@
text
@/*
 * pview.h
 *	A "view" of some pages
 *
 * One or more of these exist under a vas struct
 */
#ifndef _PVIEW_H
#define _PVIEW_H
#include <sys/types.h>

struct pview {
	struct pset *p_set;	/* Physical pages under view */
	void *p_vaddr;		/* Virtual address of view */
	uint p_len;		/* # pages in view */
	uint p_off;		/* Offset within p_set */
	struct vas *p_vas;	/* VAS we're located under */
	struct pview		/* For listing under vas */
		*p_next;
	uchar p_prot;		/* Protections on view */
};

/*
 * Bits for protection
 */
#define PROT_RO (1)		/* Read only */
#define PROT_KERN (2)		/* Kernel only */
#define PROT_MMAP (4)		/* Created by mmap() */
#define PROT_FORK (8)		/* View is in process of fork() */

#ifdef KERNEL
/*
 * Routines
 */
extern void free_pview(struct pview *);
extern struct pview *dup_pview(struct pview *),
	*copy_pview(struct pview *);
extern struct pview *alloc_pview(struct pset *);
extern void remove_pview(struct vas *, void *);

#endif /* KERNEL */
#endif /* _PVIEW_H */
@


1.3
log
@Fix warnings, prepare for -Wall
@
text
@a30 3
STRUCT_REF(pset);
STRUCT_REF(vas);

@


1.2
log
@Add flag to indicate pview is in fork().  Needed to flag this
to hat, which otherwise might take exception to some of the
addresses provided in the p_vaddr field.
@
text
@d31 3
d40 2
a41 1
#endif
d43 1
@


1.1
log
@Initial revision
@
text
@d28 1
@
