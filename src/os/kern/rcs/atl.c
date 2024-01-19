head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.5
date	94.04.19.00.27.10;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.05.00.36.02;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.04.19.39.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.50.19;	author vandys;	state Exp;
branches;
next	;


desc
@Attach List handling
@


1.5
log
@Convert to MALLOC
@
text
@/*
 * atl.c
 *	Routines for maintaining attach lists
 *
 * To implement a two-handed clock paging algorithm, you need to be
 * able to enumerate the translations which are active on a physical
 * page.  This is done using a couple levels of back links.  The
 * core entry points back to the "master" pset's slot using c_pset
 * and c_psidx.  Each slot in the pset holds a linked list of attached
 * views, starting at the p_atl field.  Finally, each COW pset can be
 * enumerated via the p_cowsets field.
 *
 * Mutexing of the p_atl field is provided by the slot lock for the
 * slot in the pset.  Mutexing for the c_pset/c_psidx is done using
 * the lock_page() interface.
 */
#include <sys/types.h>
#include <sys/pset.h>
#include <sys/assert.h>
#include <sys/malloc.h>

/*
 * add_atl()
 *	Add an attach list element for the given view
 *
 * Assumes slot for perpage is already locked.
 */
void
add_atl(struct perpage *pp, struct pview *pv, uint off)
{
	struct atl *a;

	a = MALLOC(sizeof(struct atl), MT_ATL);
	a->a_pview = pv;
	a->a_idx = off;
	a->a_next = pp->pp_atl;
	pp->pp_atl = a;
}

/*
 * delete_atl()
 *	Delete the translation for the given view
 *
 * Return value is 0 if the entry could be found; 1 if not.
 * Page slot is assumed to be held locked.
 */
delete_atl(struct perpage *pp, struct pview *pv, uint off)
{
	struct atl *a, **ap;

	/*
	 * Search for the mapping into our view
	 */
	ap = &pp->pp_atl;
	for (a = pp->pp_atl; a; a = a->a_next) {
		if ((a->a_pview == pv) && (a->a_idx == off)) {
			*ap = a->a_next;
			break;
		}
		ap = &a->a_next;
	}

	/*
	 * If we didn't find it, return failure.
	 */
	if (!a) {
		return(1);
	}

	/*
	 * Otherwise free the memory and return success.
	 */
	FREE(a, MT_ATL);
	return(0);
}
@


1.4
log
@Source reorg
@
text
@d20 1
a20 1
#include <alloc.h>
d33 1
a33 1
	a = malloc(sizeof(struct atl));
d73 1
a73 1
	free(a);
@


1.3
log
@Inappropriate assertion.  There can, of course, be multiple
views of a page--consider how messaging maps a user's buffer
into the server.
@
text
@d20 1
a20 1
#include <lib/alloc.h>
@


1.2
log
@Rewrite of VM to get page reference counts right.
@
text
@a59 2
		ASSERT_DEBUG(a->a_pview != pv,
			"delete_atl: multiple mapping?");
@


1.1
log
@Initial revision
@
text
@d7 5
a11 1
 * page.  This is done using linked lists of pviews and offsets.
d13 3
a15 2
 * Mutexing of the c_atl field is provided by the page lock for the
 * page.
d18 1
a18 1
#include <sys/core.h>
d20 1
a20 1
#include <sys/vm.h>
a21 3
extern struct core *core;
extern void *malloc();

d26 1
a26 1
 * Handles locking of page directly
d29 1
a29 1
add_atl(uint pfn, struct pview *pv, uint off)
a30 1
	struct core *c;
d36 2
a37 5
	c = &core[pfn];
	lock_page(pfn);
	a->a_next = c->c_atl;
	c->c_atl = a;
	unlock_page(pfn);
d45 1
a45 1
 * Handles locking within this routine.
d47 1
a47 1
delete_atl(uint pfn, struct pview *pv, uint off)
a48 1
	struct core *c;
a50 3
	c = &core[pfn];
	ap = &c->c_atl;

d54 2
a55 2
	lock_page(pfn);
	for (a = c->c_atl; a; a = a->a_next) {
a63 1
	unlock_page(pfn);
@
