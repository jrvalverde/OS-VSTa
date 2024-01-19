head	1.10;
access;
symbols
	V1_3_1:1.7
	V1_3:1.6
	V1_2:1.6
	V1_1:1.6
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.10
date	95.01.27.17.08.33;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	95.01.10.05.41.08;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.12.19.05.50.49;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.19.00.27.31;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.01.18.45.14;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.31.04.39.52;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.30.01.09.57;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.10.18.10.32;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.04.19.39.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.55.15;	author vandys;	state Exp;
branches;
next	;


desc
@Code for stealing memory from processes as needed
@


1.10
log
@Add a couple firewall assertions
@
text
@/*
 * vm_steal.c
 *	Routines for stealing memory when needed
 *
 * There is no such thing as swapping in VSTa.  Just page stealing.
 *
 * The basic mechanism is a two-handed clock algorithm.  This technique
 * provides a smooth mechanism for reaping pages across a wide range
 * of configurations.  However, in a multi-CPU environment it certainly
 * presents some challenges.
 *
 * The per-page information includes an attach list data structure.
 * This is used by the basic pageout algorithm to enumerate the
 * current users of a given page.  The pageout daemon enumerates
 * each page set for a page, updating the central page's notion
 * of its modifed and referenced bits.  For the forward hand, it
 * clears the referenced bit.  For the back hand, it reaps the page
 * if the referenced bit is still clear.
 *
 * Locking is tricky.  The attach list uses the page lock.  Because it
 * is taken before the slot locks in the page sets, page sets which
 * are busy must be skipped.   Especially critical is the TLB shoot-
 * down implied by the deletion of a translation in a multi-threaded
 * process.  The HAT for such an implementation will not be trivial.
 */
#include <sys/types.h>
#include <sys/mutex.h>
#include <mach/param.h>
#include <sys/pview.h>
#include <sys/pset.h>
#include <sys/core.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/malloc.h>
/* #define WATCHMEM /* */

#define CONTAINS(base, cnt, num) \
	(((num) >= (base)) && ((num) < ((base)+(cnt))))

/* Entries to avoid */
#define BAD(c) ((c)->c_flags & (C_SYS|C_BAD|C_WIRED))

/*
 * The following are in terms of 1/nth memory
 */
#define SPREAD 8		/* Distance between hands */
#define DESFREE 8		/* When less than this, start stealing */
#define MINFREE 16		/* When less than this, synch pushes */
				/* AT MOST: */
#define SMALLSCAN 32		/* Scan this much when free > DESFREE */
#define MEDSCAN 16		/*  ... when free > MINFREE */
#define LARGESCAN 4		/*  ... when free < MINFREE */

#define PAGEOUT_SECS (5)	/* Interval to run pageout() */

extern struct core *core,		/* Per-page info */
	*coreNCORE;
extern uint freemem, totalmem;		/* Free and total pages in system */
					/*  total does not include C_SYS */
sema_t pageout_sema;
uint pageout_secs = 0;		/* Set to PAGEOUT_SECS when ready */
uint desfree, minfree;		/* Initial values calculated on boot */
extern struct portref
	*swapdev;		/* To tell when it's been enabled */

/*
 * unvirt()
 *	Unvirtualize all mappings for a given slot
 *
 * Frees the attach list elements as well.
 */
static uint
unvirt(struct perpage *pp)
{
	struct atl *a, *an, **ap;
	uint flags = 0;

	ap = &pp->pp_atl;
	for (a = *ap; a; a = an) {
		struct pview *pv;

		an = a->a_next;
		pv = a->a_pview;
		if (pv->p_vas->v_flags & VF_MEMLOCK) {
			/*
			 * Leave entry be if memory locked
			 */
			ap = &a->a_next;
		} else {
			/*
			 * Delete translation, free attach list element
			 */
			hat_deletetrans(pv, (char *)pv->p_vaddr +
				ptob(a->a_idx), pp->pp_pfn);
			flags |= hat_getbits(pv,
				(char *)pv->p_vaddr + ptob(a->a_idx));
			*ap = an;
			FREE(a, MT_ATL);
			ASSERT_DEBUG(pp->pp_refs > 0, "unvirt: underflow");
			pp->pp_refs -= 1;
		}
	}
	return(flags);
}

/*
 * steal_master()
 *	Handle stealing of pages from master copy of COW
 */
steal_master(struct pset *ps, struct perpage *pp, uint idx,
	int trouble, intfun steal)
{
	struct pset *ps2;
	struct perpage *pp2;
	uint idx2;

	/*
	 * First handle direct references
	 */
	pp->pp_flags |= unvirt(pp);

	/*
	 * Walk each potential COW reference to our slot
	 */
	for (ps2 = ps->p_cowsets; ps2 && pp->pp_refs; ps2 = ps2->p_cowsets) {
		/*
		 * If the pset doesn't cover our range of the master,
		 * it can't be involved so ignore it.
		 */
		if (!CONTAINS(ps2->p_off, ps2->p_len, idx)) {
			continue;
		}

		/*
		 * Try to lock.  Since the canonical order is
		 * cow->master, we must back off on potential
		 * deadlock.
		 */
		if (cp_lock(&ps2->p_lock, SPL0)) {
			continue;
		}

		/*
		 * Examine the page slot indicated.  Again, lock with
		 * care as we're going rather backwards.
		 */
		idx2 = idx - ps2->p_off;
		pp2 = find_pp(ps2,  idx2);
		if (!clock_slot(ps2, pp2)) {
			if (pp2->pp_flags & PP_COW) {
				pp->pp_flags |= unvirt(pp2);
				if (pp2->pp_refs == 0) {
					pp2->pp_flags &= ~(PP_COW|PP_V|PP_R);
					pp->pp_refs -= 1;
				}
			}
			unlock_slot(ps2, pp2);
		} else {
			v_lock(&ps2->p_lock, SPL0);
		}
	}

	/*
	 * If he successfully stole all translations and
	 * things are getting tight, go ahead and take the memory
	 */
	ASSERT_DEBUG((pp->pp_flags & PP_M) == 0,
		"steal_master: file modified");
	if ((pp->pp_refs == 0) && (*steal)(pp->pp_flags, trouble)) {
		free_page(pp->pp_pfn);
		pp->pp_flags &= ~(PP_R|PP_V);
	}
}

/*
 * do_hand()
 *	Do hand algorithm
 *
 * Steal pages as appropriate, do all the fancy locking.
 */
static void
do_hand(struct core *c, int trouble, intfun steal)
{
	struct pset *ps;
	struct perpage *pp;
	uint idx, pgidx;
	int slot_held;
	extern void iodone_unlock();

	/*
	 * No point fussing over inactive pages, eh?
	 */
	if ((c->c_flags & C_ALLOC) == 0) {
		return;
	}

	/*
	 * Lock physical page
	 */
	pgidx = c-core;
	if (clock_page(pgidx)) {
		return;
	}

	/*
	 * Avoid bad pages and those which are changing state
	 */
	ps = c->c_pset;
	if (BAD(c) || (ps == 0)) {
		unlock_page(pgidx);
		return;
	}
	idx = c->c_psidx;

	/*
	 * Lock pset
	 */
	p_lock(&ps->p_lock, SPL0);

	/*
	 * 0 references means we've come upon this pset in the process
	 * of creation or tear-down.  Leave it alone.
	 */
	if (ps->p_refs == 0) {
		v_lock(&ps->p_lock, SPL0);
		unlock_page(pgidx);
		return;
	}

	/*
	 * Access indicated slot
	 */
	pp = find_pp(ps, idx);
	if (clock_slot(ps, pp)) {
		v_lock(&ps->p_lock, SPL0);
		unlock_page(pgidx);
		return;
	}
	slot_held = 1;

	/*
	 * The core pointer to a pset should point to the *master*
	 * pset for copy-on-write situations.
	 */
	ASSERT_DEBUG((pp->pp_flags & PP_COW) == 0,
		"do_hand: cow in set_core");

	/*
	 * If this is the target for COW psets, several assumptions
	 * can be made, so handle in its own routine.
	 */
	if ((ps->p_type != PT_COW) && ps->p_cowsets) {
		steal_master(ps, pp, idx, trouble, steal);
		goto out;
	}

	/*
	 * Scan each attached view of the slot, update our notion of
	 * page state.  Take away all translations so our notion will
	 * remain correct until we release the page slot.
	 */
	pp->pp_flags |= unvirt(pp);

	/*
	 * If there are refs, it can only mean the page is involved
	 * in a memory-locked region.  Leave it alone.
	 */
	if (pp->pp_refs > 0) {
		goto out;
	}

	/*
	 * If any trouble, see if this page is should be
	 * stolen.
	 */
	if (!(pp->pp_flags & PP_M) &&
			(*steal)(pp->pp_flags, trouble)) {
		/*
		 * Yup.  Take it away.
		 */
		free_page(pp->pp_pfn);
		pp->pp_flags &= ~(PP_V|PP_R);
	} else if (trouble && (pp->pp_flags & PP_M) && swapdev) {
		/*
		 * It's dirty, and we're interested in getting
		 * some memory soon.  Start cleaning pages.
		 */
		pp->pp_flags &= ~PP_M;
		(*(ps->p_ops->psop_writeslot))(ps, pp, idx, iodone_unlock);
		slot_held = 0;	/* Released in iodone_unlock */
	} else {
		/*
		 * Leave it alone.  Clear REF bit so we can
		 * estimate its use in the future.
		 */
		pp->pp_flags &= ~PP_R;
	}
out:
	if (slot_held) {
		unlock_slot(ps, pp);
	}
	unlock_page(pgidx);
}

/*
 * steal1()
 *	Steal algorithm for hand #1
 */
static int
steal1(int flags, int trouble)
{
	return (trouble > 1);
}

/*
 * steal2()
 *	Steal algorithm for hand #2
 */
static int
steal2(int flags, int trouble)
{
	return (trouble && !(flags & (PP_R|PP_M)));
}

/*
 * pageout()
 *	Endless routine to detect memory shortages and steal pages
 */
pageout(void)
{
	struct core *base, *top, *hand1, *hand2;
	int trouble, npg;
	int troub_cnt[3];

/*
 * Not sure if these need to be macros; have to move "top" and "base"
 * to globals to do it in its own function.
 */
#define INC(hand) {if (++(hand) >= top) hand = base; }
#define ADVANCE(hand) \
	{ INC(hand); \
		while (BAD(hand)) { \
			INC(hand); \
		} \
	}

	/*
	 * Check for mistake.  Not designed to be MP safe; "shouldn't
	 * happen", anyway.
	 */
	if (pageout_secs) {
		return(err(EBUSY));
	}
	pageout_secs = PAGEOUT_SECS;

	/*
	 * Skip to first usable page, also record top
	 */
	for (base = core; !BAD(base); ++base)
		;
	top = coreNCORE;

	/*
	 * Set clock hands
	 */
	hand1 = hand2 = base;
	for (npg = totalmem/SPREAD; npg > 0; npg -= 1) {
		ADVANCE(hand1);
	}
	npg = 0;

	/*
	 * Calculate scan counts for each situation
	 */
	troub_cnt[0] = totalmem/SMALLSCAN;
	troub_cnt[1] = totalmem/MEDSCAN;
	troub_cnt[2] = totalmem/LARGESCAN;
	minfree = totalmem/MINFREE;
	desfree = totalmem/DESFREE;

	/*
	 * Initialize our semaphore
	 */
	init_sema(&pageout_sema);

	/*
	 * Main loop
	 */
	for (;;) {
		/*
		 * Categorize the current memory situation
		 */
		if (freemem < minfree) {
			trouble = 2;
		} else if (freemem < desfree) {
			trouble = 1;
		} else {
			trouble = 0;
		}

		/*
		 * Sleep after the appropriate number of iterations
		 */
		if (npg > troub_cnt[trouble]) {
			npg = 0;
#ifdef WATCHMEM
			{ ulong ofree, odes, omin, otroub;
			if ((freemem != ofree) || (desfree != odes) ||
					(minfree != omin) ||
					(trouble != otroub)) {
			printf("pageout free %d des %d min %d trouble %d\n",
				freemem, desfree, minfree, trouble);
			ofree = freemem; odes = desfree; omin = minfree;
			otroub = trouble;
			}
			}
#endif
			p_sema(&pageout_sema, PRIHI);

			/*
			 * Clear all v's after wakeup
			 */
			set_sema(&pageout_sema, 0);
		}

		/*
		 * Do the two hand algorithms
		 */
		do_hand(hand1, trouble, steal1);
		ADVANCE(hand1);
		do_hand(hand2, trouble, steal2);
		ADVANCE(hand2);
		npg += 1;
	}
}

/*
 * kick_pageout()
 *	Wake pageout daemon periodically
 */
void
kick_pageout(void)
{
	ASSERT_DEBUG(pageout_secs > 0, "kick_pageout: no daemon");
	v_sema(&pageout_sema);
}
@


1.9
log
@Allow page stealing of pure pages before swap space is
enabled.  Enhance WATCHMEM to only log state if it changes
from last display.
@
text
@d99 1
d240 7
@


1.8
log
@Remove unneeded extern
@
text
@d63 2
d275 1
a275 1
	} else if (trouble && (pp->pp_flags & PP_M)) {
d399 4
d405 4
@


1.7
log
@Convert to MALLOC
@
text
@a36 2
extern struct portref *swapdev;

@


1.6
log
@Add memory locking on per-vas level
@
text
@d98 1
a98 1
			free(a);
@


1.5
log
@Make minfree/desfree globals so they can be tweaked.
Fix treatment of qio.
@
text
@d35 1
d75 1
a75 1
	struct atl *a, *an;
d78 2
a79 1
	for (a = pp->pp_atl; a; a = an) {
d84 17
a100 5
		hat_deletetrans(pv, (char *)pv->p_vaddr + ptob(a->a_idx),
			pp->pp_pfn);
		flags |= hat_getbits(pv, (char *)pv->p_vaddr + ptob(a->a_idx));
		free(a);
		pp->pp_refs -= 1;
a101 1
	pp->pp_atl = 0;
d151 4
a154 4
				ASSERT_DEBUG(pp2->pp_refs == 0,
					"steal_master: pp2 refs");
				pp2->pp_flags &= ~(PP_COW|PP_V|PP_R);
				pp->pp_refs -= 1;
d255 8
a262 1
	ASSERT(pp->pp_refs == 0, "do_hand: stale refs");
d285 1
a285 1
		 * No, leave it alone.  Clear REF bit so we can
@


1.4
log
@First pass at bring-up of page stealing
@
text
@a32 1
#include <sys/qio.h>
a43 2
ulong failed_qios = 0L;		/* Number pushes which failed */

a61 1
static struct qio *pgio;	/* Our asynch I/O descriptor */
d63 1
d173 1
d225 1
d262 1
d271 3
a273 1
	unlock_slot(ps, pp);
d350 2
d365 1
a365 1
		if (freemem < totalmem/MINFREE) {
d367 1
a367 1
		} else if (freemem < totalmem/DESFREE) {
d378 4
a387 7
		}

		/*
		 * Ensure a slot for asynch page push
		 */
		if (!pgio) {
			pgio = alloc_qio();
@


1.3
log
@Fix page stealing to avoid wired pages
@
text
@d58 2
d66 1
d99 2
a100 1
steal_master(struct pset *ps, struct perpage *pp, uint idx)
d106 9
a114 1
	for (ps2 = ps->p_cowsets; ps2; ps2 = ps2->p_cowsets) {
d140 1
a140 1
				(void)unvirt(pp2);
d151 11
d179 8
a186 1
	 * Lock physical page.  Point to the master pset.
d192 6
a197 1
	if (BAD(c)) {
a200 1
	ps = c->c_pset;
d204 1
a204 1
	 * Lock master slot
d207 14
d233 1
a233 10
		steal_master(ps, pp, idx);

		/*
		 * If he successfully stole all translations and
		 * things are getting tight, go ahead and take the memory
		 */
		if ((pp->pp_refs == 0) && (*steal)(pp->pp_flags, trouble)) {
			free_page(pp->pp_pfn);
			pp->pp_flags &= ~(PP_R|PP_V);
		}
d282 1
a282 1
	return ((trouble > 1) || !(flags & PP_R));
a298 1
void
d304 12
a315 1
#define ADVANCE(hand) {if (++(hand) >= top) hand = base; }
d318 9
d336 5
a340 2
	hand2 = base;
	hand1 = base + (top-base)/SPREAD;
d345 3
a347 3
	troub_cnt[0] = (top-base)/SMALLSCAN;
	troub_cnt[1] = (top-base)/MEDSCAN;
	troub_cnt[2] = (top-base)/LARGESCAN;
d375 1
d398 11
@


1.2
log
@Rewrite of VM to get page reference counts right.
@
text
@d42 3
d152 1
a152 1
	uint idx;
d158 6
a163 1
	if (clock_page(c-core)) {
d176 1
a176 1
		unlock_page(c-core);
d233 1
a233 1
	unlock_page(c-core);
d271 1
a271 1
	for (base = core; !(base->c_flags & (C_SYS|C_BAD)); ++base)
@


1.1
log
@Initial revision
@
text
@d39 3
d63 2
a64 2
 * steal_page()
 *	Remove a page from a pset
d66 26
a91 5
 * Called with page and slot locked.  On return page has been
 * removed and is perhaps queued for I/O.  The attach list entry
 * corresponding to this mapping has also been removed.  The
 * slot will either be freed on return or when the queued I/O
 * completes.
d93 1
a93 2
static void
steal_page(struct pview *pv, uint idx, struct core *c)
d95 3
a97 3
	struct pset *ps = pv->p_set;
	struct perpage *pp;
	uint setidx, nref;
d99 8
a106 7
	/*
	 * Get per-page information
	 */
	setidx = (idx - btop((ulong)pv->p_vaddr)) + pv->p_off;
	pp = find_pp(ps, setidx);
	ASSERT(pp, "steal_page: page not in set");
	ASSERT(pp->pp_flags & PP_V, "steal_page: page not present");
d108 8
a115 9
	/*
	 * Snatch away from its poor users.  Anybody who now tries
	 * to use it will fault in and wait for us to release
	 * the pset.  Bring in the authoritative copy of the ref/mod
	 * info now that our users can't touch it.
	 */
	hat_deletetrans(pv, (char *)pv->p_vaddr + ptob(idx),
		pp->pp_pfn);
	pp->pp_flags |= hat_getbits(pv, idx);
d117 19
a135 4
	/*
	 * Remove our reference.
	 */
	deref_slot(ps, pp, idx);
d147 3
a149 1
	struct atl *a, *an, **ap;
d153 10
a162 1
	 * Lock physical page.  Scan the attach list entries.
d164 7
a170 5
	lock_page(c-core);
	for (a = c->c_atl, ap = &c->c_atl; a; a = an) {
		uint idx;
		struct pset *ps;
		struct perpage *pp;
d172 6
a177 3
		an = a->a_next;
		ps = a->a_pview->p_set;
		idx = a->a_idx;
d180 6
a185 9
		 * Lock the set. cp_lock() would be too timid; I suspect
		 * that busy psets might represent most of the memory on
		 * a thrashing system.
		 */
		(void)p_lock(&ps->p_lock, SPL0);
		pp = find_pp(ps, idx);
		if (clock_slot(ps, pp)) {
			v_lock(&ps->p_lock, SPL0);
			break;
d187 10
a196 1
		ASSERT_DEBUG(pp->pp_flags & PP_V, "do_hand: atl !v");
d198 6
d205 1
a205 1
		 * Get and clear HAT copy of information
d207 10
a216 2
		pp->pp_flags |= hat_getbits(a->a_pview, idx);

d218 4
a221 31
		 * If any trouble, see if this page is should be
		 * stolen.
		 */
		if (!(pp->pp_flags & PP_M) &&
				(*steal)(pp->pp_flags, trouble)) {
			/*
			 * Yup.  Take it away and free the atl.
			 * We *should* use free_atl(), but we're
			 * already in the middle of the list, so we
			 * can do it quicker this way.
			 */
			steal_page(a->a_pview, idx, c);
			*ap = an;
			free(a);
		} else if (trouble && (pp->pp_flags & PP_M)) {
			/*
			 * It's dirty, and we're interested in getting
			 * some memory soon.  Start cleaning pages.
			 */
			pp->pp_flags &= ~PP_M;
			(*(ps->p_ops->psop_writeslot))(ps, pp, idx,
				iodone_unlock);
		} else {
			/*
			 * No, leave it alone.  Clear REF bit so we can
			 * estimate its use in the future.
			 */
			pp->pp_flags &= ~(PP_R);
			ap = &a->a_next;
			unlock_slot(ps, pp);
		}
d223 2
@
