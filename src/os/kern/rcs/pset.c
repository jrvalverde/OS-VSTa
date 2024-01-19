head	1.21;
access;
symbols
	V1_3_1:1.17
	V1_3:1.15
	V1_2:1.15
	V1_1:1.15
	V1_0:1.14;
locks; strict;
comment	@ * @;


1.21
date	95.01.27.04.43.37;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	95.01.04.06.13.42;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	94.11.16.19.36.07;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	94.10.23.17.44.12;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.04.19.19.30.59;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.04.19.00.26.44;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.05.06.23.26.37;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.03.31.04.37.51;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.03.30.01.10.27;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.03.23.18.11.21;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.03.22.20.31.41;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.03.16.19.13.05;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.03.05.23.20.10;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.02.23.18.15.26;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.02.09.17.10.00;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.05.16.04.32;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.04.19.39.16;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.03.20.14.43;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.15.16.59;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.52.52;	author vandys;	state Exp;
branches;
next	;


desc
@Common code for psets
@


1.21
log
@Add some overflow checks
@
text
@/*
 * pset.c
 *	Routines for creating/searching/filling page sets
 */
#include <sys/pset.h>
#include <sys/assert.h>
#include <sys/fs.h>
#include <sys/qio.h>
#include <sys/fs.h>
#include <sys/port.h>
#include <sys/vm.h>
#include <sys/malloc.h>
#include <alloc.h>

extern struct portref *swapdev;
extern struct psetops psop_zfod;

/*
 * find_pp()
 *	Given pset and index, return perpage information
 */
struct perpage *
find_pp(struct pset *ps, uint idx)
{
	ASSERT_DEBUG(idx < ps->p_len, "find_pp: bad index");
	ASSERT_DEBUG(ps->p_perpage, "find_pp: no perpage");
	return(&ps->p_perpage[idx]);
}

/*
 * free_pset()
 *	Release a page set; update pages it references
 *
 * This routines assumes the drudgery of getting the pset free from
 * its pviews has been handled.  In particular, our caller is assumed to
 * have waited until any asynch I/O is completed.
 */
void
free_pset(struct pset *ps)
{
	ASSERT_DEBUG(ps->p_refs == 0, "free_pset: refs > 0");
	ASSERT(ps->p_locks == 0, "free_pset: locks > 0");

	/*
	 * Free pages under pset.  Memory psets are not "real", and
	 * thus you can't free the pages under them.
	 */
	if (ps->p_type != PT_MEM) {
		int x;
		struct perpage *pp;

		pp = ps->p_perpage;
		for (x = 0; x < ps->p_len; ++x,++pp) {
			ASSERT_DEBUG(pp->pp_refs == 0,
				"free_pset: still refs");
			/*
			 * Non-valid slots--no problem
			 */
			if ((pp->pp_flags & PP_V) == 0) {
				continue;
			}

			/*
			 * Release reference to underlying pset's slot
			 */
			if (pp->pp_flags & PP_COW) {
				struct perpage *pp2;
				uint idx;
				struct pset *ps2;

				ps2 = ps->p_cow;
				idx = ps->p_off + x;
				p_lock(&ps2->p_lock, SPL0);
				pp2 = find_pp(ps2, idx);
				lock_slot(ps2, pp2);
				deref_slot(ps2, pp2, idx);
				unlock_slot(ps2, pp2);
				continue;
			}

			/*
			 * Free page
			 */
			free_page(pp->pp_pfn);
		}

		/*
		 * Release our swap space
		 */
		free_swap(ps->p_swapblk, ps->p_len);
	}

	/*
	 * Free our reference to the master set on PT_COW
	 */
	if (ps->p_type == PT_COW) {
		struct pset *ps2 = ps->p_cow, **pp, *p;

		/*
		 * Remove us from his COW list
		 */
		p_lock(&ps2->p_lock, SPL0);
		pp = &ps2->p_cowsets;
		for (p = ps2->p_cowsets; p; p = p->p_cowsets) {
			if (p == ps) {
				*pp = p->p_cowsets;
				break;
			}
			pp = &p->p_cowsets;
		}
		ASSERT(p, "free_pset: lost cow");
		v_lock(&ps2->p_lock, SPL0);

		/*
		 * Remove our reference from him
		 */
		deref_pset(ps2);

	/*
	 * Need to disconnect from our server on mapped files
	 */
	} else if (ps->p_type == PT_FILE) {
		(void)shut_client(ps->p_pr);
	}

	/*
	 * Release our pset itself
	 */
	FREE(ps->p_perpage, MT_PERPAGE);
	FREE(ps, MT_PSET);
}

/*
 * physmem_pset()
 *	Create a pset which holds a view of physical memory
 */
struct pset *
physmem_pset(uint pfn, int npfn)
{
	struct pset *ps;
	struct perpage *pp;
	int x;
	extern struct psetops psop_mem;

	/*
	 * Initialize the basic fields of the pset
	 */
	ps = MALLOC(sizeof(struct pset), MT_PSET);
	ps->p_perpage = MALLOC(npfn * sizeof(struct perpage), MT_PERPAGE);
	ps->p_len = npfn;
	ps->p_off = 0;
	ps->p_type = PT_MEM;
	ps->p_swapblk = 0;
	ps->p_refs = 0;
	ps->p_cowsets = 0;
	ps->p_ops = &psop_mem;
	init_lock(&ps->p_lock);
	ps->p_locks = 0;
	init_sema(&ps->p_lockwait);

	/*
	 * For each page slot, put in the physical page which
	 * corresponds to the slot.
	 */
	for (x = 0; x < npfn; ++x) {
		pp = find_pp(ps, x);
		pp->pp_pfn = pfn+x;
		pp->pp_flags = PP_V;
		pp->pp_refs = 0;
		pp->pp_lock = 0;
		pp->pp_atl = 0;
	}
	return(ps);
}

/*
 * lock_slot()
 *	Lock a particular slot within the pset
 *
 * Caller is assumed to hold lock on pset.  This routine may sleep
 * waiting for the slot to become unlocked.  On return, the slot
 * is locked, and the pset lock is released.
 */
void
lock_slot(struct pset *ps, struct perpage *pp)
{
	/*
	 * If we have the poor luck to collide with a parallel operation,
	 * wait for it to finish.
	 */
	ps->p_locks += 1;
	while (pp->pp_lock & PP_LOCK) {
		ASSERT(ps->p_locks > 1, "lock_slot: stray lock");
		pp->pp_lock |= PP_WANT;
		p_sema_v_lock(&ps->p_lockwait, PRIHI, &ps->p_lock);
		(void)p_lock(&ps->p_lock, SPL0);
	}
	pp->pp_lock |= PP_LOCK;
	v_lock(&ps->p_lock, SPL0);
}

/*
 * clock_slot()
 *	Like lock_slot(), but don't block if slot busy
 */
clock_slot(struct pset *ps, struct perpage *pp)
{
	if (pp->pp_lock & PP_LOCK) {
		/*
		 * Sorry, busy
		 */
		return(1);
	}

	/*
	 * Take the lock
	 */
	ps->p_locks += 1;
	pp->pp_lock |= PP_LOCK;
	v_lock(&ps->p_lock, SPL0);
	return(0);
}

/*
 * unlock_slot()
 *	Release previously held slot, wake any waiters
 */
void
unlock_slot(struct pset *ps, struct perpage *pp)
{
	int wanted;

	ASSERT_DEBUG(pp->pp_lock & PP_LOCK, "unlock_slot: not locked");
	(void)p_lock(&ps->p_lock, SPL0);
	ps->p_locks -= 1;
	wanted = (pp->pp_lock & PP_WANT);
	pp->pp_lock &= ~(PP_LOCK|PP_WANT);
	if (wanted && blocked_sema(&ps->p_lockwait)) {
		vall_sema(&ps->p_lockwait);
	}
	v_lock(&ps->p_lock, SPL0);
}

/*
 * pset_writeslot()
 *	Generic code for flushing to a swap page
 *
 * Shared by COW, ZFOD, and FOD pset types.  For async, page slot will
 * be released on I/O completion.  Otherwise page is synchronously written
 * and slot is still held on return.
 */
pset_writeslot(struct pset *ps, struct perpage *pp, uint idx, voidfun iodone)
{
	struct qio *q;

	ASSERT_DEBUG(pp->pp_flags & PP_V, "nof_writeslot: invalid");
	pp->pp_flags &= ~(PP_M);
	pp->pp_flags |= PP_SWAPPED;
	if (!iodone) {
		/*
		 * Simple synchronous page push
		 */
		if (pageio(pp->pp_pfn, swapdev,
				ptob(idx+ps->p_swapblk), NBPG, FS_ABSWRITE)) {
			pp->pp_flags |= PP_BAD;
			return(1);
		}
		return(0);
	}

	/*
	 * Asynch I/O
	 */
	q = alloc_qio();
	q->q_port = swapdev;
	q->q_op = FS_ABSWRITE;
	q->q_pset = ps;
	q->q_pp = pp;
	q->q_off = ptob(idx + ps->p_swapblk);
	q->q_cnt = NBPG;
	q->q_iodone = iodone;
	qio(q);
	return(0);
}

/*
 * ref_pset()
 *	Add a reference to a pset
 */
void
ref_pset(struct pset *ps)
{
	ATOMIC_INCW(&ps->p_refs);
}

/*
 * deref_pset()
 *	Reduce reference count on pset
 *
 * Must free the pset when reference count reaches 0
 */
void
deref_pset(struct pset *ps)
{
	uint refs;

	ASSERT_DEBUG(ps->p_refs > 0, "deref_pset: 0 ref");

	/*
	 * Lock the page set, reduce its reference count
	 */
	p_lock(&ps->p_lock, SPL0);
	ATOMIC_DECW(&ps->p_refs);
	refs = ps->p_refs;
	v_lock(&ps->p_lock, SPL0);

	/*
	 * When it reaches 0, ask for it to be freed
	 */
	if (refs == 0) {
		(*(ps->p_ops->psop_deinit))(ps);
		free_pset(ps);
	}
}

/*
 * iodone_unlock()
 *	Simply release slot lock on I/O completion
 */
void
iodone_unlock(struct qio *q)
{
	struct perpage *pp = q->q_pp;

	pp->pp_flags &= ~(PP_R|PP_M);
	unlock_slot(q->q_pset, pp);
}

/*
 * alloc_pset()
 *	Common code for allocation of all types of psets
 *
 * Caller is responsible for any swap allocations.
 */
static struct pset *
alloc_pset(uint pages)
{
	struct pset *ps;

	ps = MALLOC(sizeof(struct pset), MT_PSET);
	bzero(ps, sizeof(struct pset));
	ps->p_len = pages;
	ps->p_perpage = MALLOC(sizeof(struct perpage) * pages, MT_PERPAGE);
	bzero(ps->p_perpage, sizeof(struct perpage) * pages);
	init_lock(&ps->p_lock);
	init_sema(&ps->p_lockwait); set_sema(&ps->p_lockwait, 0);
	return(ps);
}

/*
 * alloc_pset_zfod()
 *	Allocate a generic pset with all invalid pages
 */
struct pset *
alloc_pset_zfod(uint pages)
{
	struct pset *ps;
	uint swapblk;

	/*
	 * Get backing store first
	 */
	if ((swapblk = alloc_swap(pages)) == 0) {
		return(0);
	}

	/*
	 * Allocate pset, set it for our pset type
	 */
	ps = alloc_pset(pages);
	ps->p_type = PT_ZERO;
	ps->p_ops = &psop_zfod;
	ps->p_swapblk = swapblk;

	return(ps);
}

/*
 * alloc_pset_fod()
 *	Allocate a fill-on-demand pset with all invalid pages
 */
struct pset *
alloc_pset_fod(struct portref *pr, uint pages)
{
	struct pset *ps;
	extern struct psetops psop_fod;

	/*
	 * Allocate pset, set it for our pset type
	 */
	ps = alloc_pset(pages);
	ps->p_type = PT_FILE;
	ps->p_ops = &psop_fod;
	ps->p_pr = pr;
	return(ps);
}

/*
 * copy_page()
 *	Make a copy of a page
 */
static void
copy_page(uint idx, struct perpage *opp, struct perpage *pp,
	struct pset *ops, struct pset *ps)
{
	uint pfn;

	pfn = alloc_page();
	set_core(pfn, ps, idx);
	if (opp->pp_flags & PP_V) {
		/*
		 * Valid means simple memory->memory copy
		 */
		bcopy(ptov(ptob(opp->pp_pfn)), ptov(ptob(pfn)), NBPG);

	/*
	 * Otherwise read from swap
	 */
	} else {
		ASSERT(opp->pp_flags & PP_SWAPPED, "copy_page: !v !swap");
		if (pageio(pfn, swapdev, ptob(idx+ops->p_swapblk),
				NBPG, FS_ABSWRITE)) {
			/*
			 * The I/O failed.  Mark the slot as bad.  Our
			 * new set is in for a rough ride....
			 */
			pp->pp_flags |= PP_BAD;
			free_page(pfn);
			return;
		}
	}
	pp->pp_flags |= PP_V;
	pp->pp_pfn = pfn;
}

/*
 * dup_slots()
 *	Duplicate the contents of each slot under an old pset
 */
static void
dup_slots(struct pset *ops, struct pset *ps)
{
	uint x;
	struct perpage *pp, *pp2;
	int locked = 0;

	for (x = 0; x < ps->p_len; ++x) {
		if (!locked) {
			p_lock(&ops->p_lock, SPL0);
			locked = 1;
		}
		pp = find_pp(ops, x);
		pp2 = find_pp(ps, x);
		if (pp->pp_flags & (PP_V|PP_SWAPPED)) {
			lock_slot(ops, pp);
			locked = 0;

			/*
			 * COW views can be filled from the underlying
			 * set on demand.
			 */
			if ((pp->pp_flags & (PP_V|PP_COW)) !=
					(PP_V|PP_COW)) {
				/*
				 * Valid page, need to copy.
				 *
				 * The state can have changed as we may
				 * have slept on the slot lock.  But
				 * there's still something in memory or
				 * on swap to copy--copy_page() handles
				 * both cases.
				 */
				copy_page(x, pp, pp2, ops, ps);
				unlock_slot(ops, pp);
			}
		}
	}

	/*
	 * Drop any trailing lock
	 */
	if (locked) {
		v_lock(&ops->p_lock, SPL0);
	}
}

/*
 * add_cowset()
 *	Add a new pset to the list of COW sets under a master
 */
static void
add_cowset(struct pset *pscow, struct pset *ps)
{
	/*
	 * Attach to the underlying pset
	 */
	ref_pset(pscow);
	p_lock(&pscow->p_lock, SPL0);
	ps->p_cowsets = pscow->p_cowsets;
	pscow->p_cowsets = ps;
	ps->p_cow = pscow;
	v_lock(&pscow->p_lock, SPL0);
}

/*
 * copy_pset()
 *	Copy one pset into another
 *
 * Invalid pages can be left as-is.  Valid, unmodified, unswapped pages
 * can become invalid in copy.  Other valid pages must be copied.  Worst
 * is that invalid, swapped pages must be paged back into a new page
 * under the new pset.  Sometimes true copy-on-write sounds pretty good.
 * But it's too complex, IMHO.
 */
struct pset *
copy_pset(struct pset *ops)
{
	struct pset *ps;

	/*
	 * Allocate the new pset, set up its basic fields
	 */
	ps = MALLOC(sizeof(struct pset), MT_PSET);
	ps->p_len = ops->p_len;
	ps->p_off = ops->p_off;
	ps->p_type = ops->p_type;
	ps->p_locks = 0;
	ps->p_refs = 0;
	ps->p_flags = ops->p_flags;
	ps->p_ops = ops->p_ops;
	init_lock(&ps->p_lock);
	init_sema(&ps->p_lockwait); set_sema(&ps->p_lockwait, 0);

	switch (ps->p_type) {
	case PT_FILE:
		/*
		 * We are a new reference into the file.  Ask
		 * the server for duplication.
		 */
		ps->p_pr = dup_port(ops->p_pr);
		break;
	case PT_COW:
		/*
		 * This is a new COW reference into the master pset
		 */
		add_cowset(ops->p_cow, ps);
		break;
	case PT_MEM:
		/*
		 * Map fork() of memory based psets (ala boot processes)
		 * into a ZFOD pset which happens to be filled.  The mem
		 * pset doesn't have swap, so we get it here.
		 */
		ps->p_type = PT_ZERO;
		ps->p_ops = &psop_zfod;
		ps->p_swapblk = alloc_swap(ps->p_len);
		break;
	}

	/*
	 * We need our own perpage storage
	 */
	ps->p_perpage = MALLOC(ops->p_len * sizeof(struct perpage),
		MT_PERPAGE);
	bzero(ps->p_perpage, ps->p_len * sizeof(struct perpage));

	/*
	 * If old copy had swap, get swap for new copy
	 */
	if (ops->p_swapblk != 0) {
		ps->p_swapblk = alloc_swap(ps->p_len);
	}

	/*
	 * Now duplicate the contents
	 */
	dup_slots(ops, ps);
	return(ps);
}

/*
 * deref_slot()
 *	Decrement reference count on a page slot, free page on last ref
 *
 * This routine assumes that it is being called under a locked slot.
 */
void
deref_slot(struct pset *ps, struct perpage *pp, uint idx)
{
	ASSERT_DEBUG(pp->pp_refs > 0, "deref_slot: zero");
	pp->pp_refs -= 1;
}

/*
 * ref_slot()
 *	Add a reference to a page slot
 *
 * Assumes caller holds the page slot locked.
 */
void
ref_slot(struct pset *ps, struct perpage *pp, uint idx)
{
	pp->pp_refs += 1;
	ASSERT_DEBUG(pp->pp_refs > 0, "ref_slot: overflow");
}

/*
 * pset_deinit()
 *	Common code to de-init a pset
 *
 * Currently does nothing.
 */
pset_deinit(struct pset *ps)
{
	return(0);
}

/*
 * alloc_pset_cow()
 *	Allocate a COW pset in terms of another
 */
struct pset *
alloc_pset_cow(struct pset *psold, uint off, uint len)
{
	struct pset *ps;
	uint swapblk;
	extern struct psetops psop_cow;

	ASSERT_DEBUG(psold->p_type != PT_COW, "pset_cow: cow of cow");

	/*
	 * Get swap for our pages.  We get all the room we'll need
	 * if all pages are written.
	 */
	swapblk = alloc_swap(len);
	if (swapblk == 0) {
		return(0);
	}
	ps = alloc_pset(len);
	ps->p_off = off;
	ps->p_swapblk = swapblk;
	ps->p_type = PT_COW;
	ps->p_ops = &psop_cow;
	add_cowset(psold, ps);

	return(ps);
}
@


1.20
log
@Don't preserve valid/COW slots in new copy--leave it entirely
invalid and let them fault it into their view.  Fixes bug
where a ref was left on the cow slot even though nothing was
attached.
@
text
@d600 1
d614 1
@


1.19
log
@Tidy up semaphore count handling, add assertions.  Convert
atomic ops so routine matches data element size.
@
text
@d469 2
a470 1
			 * COW views can be shared
d472 1
a472 1
			if ((pp->pp_flags & (PP_V|PP_COW)) ==
a473 6
				ASSERT_DEBUG(ps->p_type == PT_COW,
					"dup_slots: !cow");
				pp2->pp_pfn = pp->pp_pfn;
				pp2->pp_flags = PP_V|PP_COW;
				ref_slot(ps->p_cow, pp2, x);
			} else {
@


1.18
log
@Delete some dead code
@
text
@d293 1
a293 1
	ATOMIC_INC(&ps->p_refs);
d313 1
a313 1
	ATOMIC_DEC(&ps->p_refs);
@


1.17
log
@Add sanity check for no stale refs
@
text
@a326 17
 * iodone_free()
 *	Common function to free a page on I/O completion
 *
 * Also updates the perpage information before unlocking the page slot
 */
void
iodone_free(struct qio *q)
{
	struct perpage *pp;

	pp = q->q_pp;
	pp->pp_flags &= ~(PP_V|PP_M|PP_R);
	free_page(pp->pp_pfn);
	unlock_slot(q->q_pset, pp);
}

/*
@


1.16
log
@Convert to MALLOC
@
text
@d54 2
@


1.15
log
@Source reorg
@
text
@d12 1
d127 2
a128 2
	free(ps->p_perpage);
	free(ps);
d146 2
a147 2
	ps = malloc(sizeof(struct pset));
	ps->p_perpage = malloc(npfn * sizeof(struct perpage));
d365 1
a365 1
	ps = malloc(sizeof(struct pset));
d368 1
a368 1
	ps->p_perpage = malloc(sizeof(struct perpage) * pages);
d553 1
a553 1
	ps = malloc(sizeof(struct pset));
d593 2
a594 1
	ps->p_perpage = malloc(ops->p_len * sizeof(struct perpage));
@


1.14
log
@Forgot to advance tail pointer as we walked the chain.
@
text
@d12 1
a12 1
#include <lib/alloc.h>
@


1.13
log
@Add debug assert, use ABSWRITE in pageio
@
text
@d106 1
@


1.12
log
@Add missing return value
@
text
@d229 1
d272 1
a272 1
	q->q_op = FS_WRITE;
@


1.11
log
@Do a more thorough job of initializing psets.  Also change the
name of the parameter to our COW handling routine, and call
it with the right target COW pset.
@
text
@d217 1
@


1.10
log
@Dup of COW pset needs to update master pset's COW list.
Share code with creation of new COW pset.
@
text
@d353 2
d362 1
a363 5
	ps->p_off = 0;
	ps->p_locks = 0;
	ps->p_refs = 0;
	ps->p_cowsets = 0;
	ps->p_swapblk = 0;	/* Caller has to allocate */
d518 1
a518 1
add_cowset(struct pset *psold, struct pset *ps)
d523 6
a528 6
	ref_pset(psold);
	p_lock(&psold->p_lock, SPL0);
	ps->p_cowsets = psold->p_cowsets;
	psold->p_cowsets = ps;
	ps->p_cow = psold;
	v_lock(&psold->p_lock, SPL0);
d572 1
a572 1
		add_cowset(ops, ps);
@


1.9
log
@Forgot to lock lower COW pset when moving to lock a slot
within.
@
text
@d516 18
d568 1
a568 1
		ps->p_pr = dup_port(ps->p_pr);
d574 1
a574 1
		ref_pset(ps->p_cow);
a667 1
	ps->p_cow = psold;
d669 1
a669 9

	/*
	 * Attach to the underlying pset
	 */
	ref_pset(psold);
	p_lock(&psold->p_lock, SPL0);
	ps->p_cowsets = psold->p_cowsets;
	psold->p_cowsets = ps;
	v_lock(&psold->p_lock, SPL0);
@


1.8
log
@Add special case for PT_MEM regions on copy.  Case statement
suggests it should've been in the pset layer... think about this.
@
text
@d70 1
@


1.7
log
@Add support for COW psets
@
text
@d15 1
a380 1
	extern struct psetops psop_zfod;
d543 6
a548 5
	/*
	 * For files, we must get our own portref.  For copy-on-write,
	 * we must add a reference to the master pset.
	 */
	if (ps->p_type == PT_FILE) {
d550 5
a554 1
	} else if (ps->p_type == PT_COW) {
d556 11
@


1.6
log
@Don't leave lock on pset, even when discarding.  Throws
off the pc_locks count.
@
text
@d81 5
d112 6
a380 1
	extern uint alloc_swap();
d401 20
d606 40
@


1.5
log
@Add cleanup of p_cowsets linked list on deref of COW pset
@
text
@d286 2
d295 2
d301 1
a301 1
	if (ps->p_refs == 0) {
a303 5
	} else {
		/*
		 * Otherwise let it go
		 */
		v_lock(&ps->p_lock, SPL0);
@


1.4
log
@Rewrite of VM to get page reference counts right.
@
text
@d87 20
a106 1
		deref_pset(ps->p_cow);
a297 1
		printf("free pset 0x%x\n", ps); dbg_enter();
@


1.3
log
@Add debugging
@
text
@d31 4
a38 3
	int x;
	struct perpage *pp;

a41 1
#ifdef DEBUG
d43 2
a44 2
	 * Make sure there are no valid slots now that all the
	 * views have been detached.
d47 33
a79 3
		for (x = 0, pp = ps->p_perpage; x < ps->p_len; ++x, ++pp) {
			ASSERT(!(pp->pp_flags & PP_V),
				"free_pset: valid pages");
a81 1
#endif
d89 4
d119 1
d135 1
d279 2
a280 2
		printf("free pset 0x%x\n", ps);
		dbg_enter();
a290 33
 * pset_unref()
 *	Release a reference to a slot
 *
 * Generic code shared by objects which move dirty pages to swap--
 * zfod, nfod, cow.
 */
pset_unref(struct pset *ps, struct perpage *pp, uint idx)
{
	extern void iodone_free();

	/*
	 * Take no further action if more refs
	 */
	if ((pp->pp_refs -= 1) > 0) {
		return;
	}

	/*
	 * Flush slot if dirty
	 */
	if ((ps->p_refs > 0) && (pp->pp_flags & PP_M)) {
		(*(ps->p_ops->psop_writeslot))(ps, pp, idx, iodone_free);
		return;
	}

	/*
	 * Release page, clear slot
	 */
	free_page(pp->pp_pfn);
	pp->pp_flags &= ~(PP_V|PP_M|PP_R);
}

/*
d334 1
d378 2
a379 1
copy_page(uint idx, struct perpage *opp, struct perpage *pp, struct pset *ps)
d384 1
d396 1
a396 1
		if (pageio(pfn, swapdev, ptob(idx+ps->p_swapblk),
d432 1
d434 1
a434 5
			 * The state can have changed as we may
			 * have slept on the slot lock.  But
			 * there's still something in memory or
			 * on swap to copy--copy_page() handles
			 * both cases.
d436 20
a455 2
			copy_page(x, pp, pp2, ps);
			unlock_slot(ops, pp);
d524 35
@


1.2
log
@Fix some dangling/missing lock problems in the loop
@
text
@d232 2
@


1.1
log
@Initial revision
@
text
@d413 1
a414 1
	p_lock(&ops->p_lock, SPL0);
d416 4
d424 1
d435 7
@
