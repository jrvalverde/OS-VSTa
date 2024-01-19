head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.3
date	93.03.30.01.08.42;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.15.13.28;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.14.22;	author vandys;	state Exp;
branches;
next	;


desc
@Generic definitions for Hardware Address Translation layer
@


1.3
log
@Add prototype for hat_getbits
@
text
@#ifndef _HAT_H
#define _HAT_H
/*
 * hat.h
 *	Definitions for Hardware Address Translation routines
 */
#include <sys/types.h>
#include <sys/vas.h>
#include <sys/pview.h>
#include <mach/hat.h>

/*
 * hat_vtop()
 *	Given virtual address and address space, return physical address
 *
 * Returns 0 if a translation does not exist
 */
extern void *hat_vtop(struct vas *, void *);

/*
 * hat_initvas()
 *	Initialize a vas
 */
extern void hat_initvas(struct vas *);

/*
 * hat_addtrans()
 *	Add a translation
 */
extern void hat_addtrans(struct pview *pv, void *vaddr, uint pfn, int write);

/*
 * hat_deletetrans()
 *	Delete a translation
 */
extern void hat_deletetrans(struct pview *pv, void *vaddr, uint pfn);

/*
 * hat_attach()
 *	Try to attach a new view
 */
extern int hat_attach(struct pview *pv, void *vaddr);

/*
 * hat_detach()
 *	Detach existing view
 */
extern void hat_detach(struct pview *pv);

/*
 * hat_fork()
 *	Hook for handling details of fork()'ing a vas
 */
extern void hat_fork(struct vas *, struct vas *);

/*
 * hat_getbits()
 *	Get bits from mapping, clear bits from HAT layer simultaneously
 */
extern int hat_getbits(struct pview *, void *);

#endif /* _HAT_H */
@


1.2
log
@Add hat_fork() entry.  Make procedure declarations extern.
@
text
@d56 6
@


1.1
log
@Initial revision
@
text
@d18 1
a18 1
void *hat_vtop(struct vas *, void *);
d24 1
a24 1
void hat_initvas(struct vas *);
d30 1
a30 1
void hat_addtrans(struct pview *pv, void *vaddr, uint pfn, int write);
d36 1
a36 1
void hat_deletetrans(struct pview *pv, void *vaddr, uint pfn);
d42 1
a42 1
int hat_attach(struct pview *pv, void *vaddr);
d48 7
a54 1
void hat_detach(struct pview *pv);
@
