head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.4
date	94.04.26.21.38.24;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.01.18.47.27;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.15.14.05;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.17.51;	author vandys;	state Exp;
branches;
next	;


desc
@Virtual. Address Space definition
@


1.4
log
@Add boot server flag
@
text
@/*
 * vas.h
 *	Definitions of a Virtual Address Space
 */
#ifndef _VAS_H
#define _VAS_H
#include <sys/mutex.h>
#include <mach/hat.h>
#include <sys/pview.h>

struct vas {
	struct pview		/* List of views in address space */
		*v_views;
	lock_t v_lock;		/* Mutex */
	uint v_flags;		/* Flags */
	struct hatvas
		v_hat;		/* Hardware-dependent stuff */
};

/*
 * Bits in v_flags
 */
#define VF_DMA 1		/* VAS used by DMA server */
#define VF_MEMLOCK 2		/* Don't page from this VAS */
#define VF_BOOT 4		/* A VAS created for a boot server */

#ifdef KERNEL
extern struct pview *detach_pview(struct vas *, void *),
	*find_pview(struct vas *, void *);
extern void *attach_pview(struct vas *, struct pview *);
extern void remove_pview(struct vas *, void *);
extern struct pview *find_ivew(struct vas *, void *);
extern void free_vas(struct vas *);
extern void *alloc_zfod(struct vas *, uint),
	*alloc_zfod_vaddr(struct vas *, uint, void *);
#endif /* KERNEL */

#endif /* _VAS_H */
@


1.3
log
@Add memory lock flag to vas
@
text
@d25 1
@


1.2
log
@Add vas flags, and flag for indicating a DMA-capable vas.
Needed to allow DMA servers to write user messages, which
are usually read-only.
@
text
@d24 1
@


1.1
log
@Initial revision
@
text
@d15 1
d19 5
@
