head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.5
date	94.04.19.00.27.31;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.05.00.37.21;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.23.18.13.41;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.04.19.39.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.52.59;	author vandys;	state Exp;
branches;
next	;


desc
@Copy On Write pset routines
@


1.5
log
@Convert to MALLOC
@
text
@/*
 * pset_cow.c
 *	Routines for implementing copy-on-write psets
 */
#include <sys/types.h>
#include <sys/pview.h>
#include <sys/pset.h>
#include <sys/qio.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/assert.h>

extern struct portref *swapdev;
extern int pset_writeslot(), pset_deinit();

static int cow_fillslot(), cow_writeslot(), cow_init();
struct psetops psop_cow = {cow_fillslot, cow_writeslot, cow_init,
	pset_deinit};

/*
 * cow_init()
 *	Set up pset for a COW view of another pset
 */
static
cow_init(struct pset *ps)
{
	return(0);
}

/*
 * cow_fillslot()
 *	Fill pset slot from the underlying master copy
 */
static
cow_fillslot(struct pset *ps, struct perpage *pp, uint idx)
{
	struct perpage *pp2;
	uint idx2;
	uint pg;

	ASSERT_DEBUG(!(pp->pp_flags & (PP_V|PP_BAD)),
		"cow_fillslot: valid");

	/*
	 * If we're on swap, bring in from there
	 */
	if (pp->pp_flags & PP_SWAPPED) {
		pg = alloc_page();
		set_core(pg, ps, idx);
		if (pageio(pg, swapdev, ps->p_swapblk + idx,
				NBPG, FS_ABSREAD)) {
			free_page(pg);
			return(1);
		}
	} else {
		struct pset *cow = ps->p_cow;

		/*
		 * Lock slot of underlying pset
		 */
		idx2 = ps->p_off + idx;
		p_lock(&cow->p_lock, SPL0);
		pp2 = find_pp(cow, idx2);
		lock_slot(cow, pp2);

		/*
		 * If the memory isn't available, call its
		 * slot filling routine.
		 */
		if (!(pp2->pp_flags & PP_V)) {
			if ((*(cow->p_ops->psop_fillslot))
					(cow, pp2, idx2)) {
				unlock_slot(cow, pp2);
				return(1);
			}
			ASSERT_DEBUG(pp2->pp_flags & PP_V,
				"cow_fillslot: cow fill !v");
		} else {
			pp2->pp_refs += 1;
		}

		/*
		 * Always initially fill with sharing of page.  We'll
		 * break the sharing and copy soon, if needed.
		 */
		pg = pp2->pp_pfn;
		pp->pp_flags |= PP_COW;
		unlock_slot(cow, pp2);
	}

	/*
	 * Fill in the new page's value
	 */
	pp->pp_refs = 1;
	pp->pp_flags |= PP_V;
	pp->pp_flags &= ~(PP_M|PP_R);
	pp->pp_pfn = pg;
	return(0);
}

/*
 * cow_writeslot()
 *	Write pset slot out to swap as needed
 *
 * The caller may sleep even with async set to true, if the routine has
 * to sleep waiting for a qio element.  The routine is called with the
 * slot locked.  On non-async return, the slot is still locked.  For
 * async I/O, the slot is unlocked on I/O completion.
 */
static
cow_writeslot(struct pset *ps, struct perpage *pp, uint idx, voidfun iodone)
{
	return(pset_writeslot(ps, pp, idx, iodone));
}

/*
 * cow_write()
 *	Do the writing-cow action
 *
 * Copies the current contents, frees reference to the master copy,
 * and switches page slot to new page.
 */
void
cow_write(struct pset *ps, struct perpage *pp, uint idx)
{
	struct perpage *pp2;
	uint idx2 = ps->p_off + idx;
	uint pg;

	pp2 = find_pp(ps->p_cow, idx2);
	pg = alloc_page();
	set_core(pg, ps, idx);
	ASSERT(pp2->pp_flags & PP_V, "cow_write: !v");
	bcopy(ptov(ptob(pp2->pp_pfn)), ptov(ptob(pg)), NBPG);
	deref_slot(ps->p_cow, pp2, idx2);
	pp->pp_pfn = pg;
	pp->pp_flags &= ~PP_COW;
}
@


1.4
log
@Forgot to flag page COW when first filled into slot
@
text
@a12 1
extern void *malloc();
@


1.3
log
@Fix locking of slot, also use a local variable to save some
dereferences.
@
text
@d77 2
d88 1
@


1.2
log
@Rewrite of VM to get page reference counts right.
@
text
@d57 2
d63 3
a65 2
		pp2 = find_pp(ps->p_cow, idx2);
		lock_slot(ps->p_cow, pp2);
d72 3
a74 3
			if ((*(ps->p_cow->p_ops->psop_fillslot))
					(ps->p_cow, pp2, idx2)) {
				unlock_slot(ps->p_cow, pp2);
d86 1
@


1.1
log
@Initial revision
@
text
@d15 1
a15 1
extern int pset_writeslot();
d17 1
a17 2
static int cow_fillslot(), cow_writeslot(), cow_init(), cow_deinit(),
	cow_unref();
d19 1
a19 1
	cow_deinit, cow_unref};
a31 10
 * cow_deinit()
 *	Tear down the goodies we set up in cow_init()
 */
static
cow_deinit(struct pset *ps)
{
	return(0);
}

/*
d50 1
d126 2
a131 46
}

/*
 * cow_unref()
 *	Remove a COW reference
 */
cow_unref(struct pset *ps, struct perpage *pp, uint idx)
{
	extern void iodone_free();

	/*
	 * If more references within pset, don't bother.
	 */
	if ((pp->pp_refs -= 1) > 0) {
		return;
	}

	/*
	 * If page is still in PP_COW state, then we simply have to
	 * remove our reference from the underlying master copy.
	 */
	if (pp->pp_flags & PP_COW) {
		struct perpage *pp2;
		uint idx2;

		ASSERT_DEBUG(!(pp->pp_flags & PP_M), "cow_unref: cow dirty");
		idx2 = ps->p_off + idx;
		pp2 = find_pp(ps->p_cow, idx2);
		(*(ps->p_cow->p_ops->psop_unrefslot))(ps->p_cow, pp2, idx2);
	} else {
		/*
		 * Otherwise we need to push dirty, or free the page.
		 * Dirty will be freed on I/O completion.
		 */
		if ((ps->p_refs > 0) && (pp->pp_flags & PP_M)) {
			(*(ps->p_ops->psop_writeslot))(ps, pp, idx,
				iodone_free);
			return;
		}
		free_page(pp->pp_pfn);
	}

	/*
	 * Flag an invalid, clean slot
	 */
	pp->pp_flags &= ~(PP_M|PP_R|PP_V);
@
