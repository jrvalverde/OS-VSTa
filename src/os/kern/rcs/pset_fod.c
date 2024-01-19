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
date	93.11.16.02.44.14;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.04.19.39.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.53.10;	author vandys;	state Exp;
branches;
next	;


desc
@Fill On Demand (from file) pset routines
@


1.3
log
@Source reorg
@
text
@/*
 * pset_fod.c
 *	Routines for implementing fill-on-demand (from file) psets
 */
#include <sys/types.h>
#include <sys/pview.h>
#include <sys/pset.h>
#include <sys/qio.h>
#include <sys/fs.h>
#include <sys/assert.h>
#include <alloc.h>

extern int pset_deinit();
static int fod_fillslot(), fod_writeslot(), fod_init(), fod_deinit();
struct psetops psop_fod = {fod_fillslot, fod_writeslot, fod_init,
	fod_deinit};

/*
 * fod_init()
 *	Set up pset for mapping a port
 */
static
fod_init(struct pset *ps)
{
	return(0);
}

/*
 * fod_deinit()
 *	Tear down the goodies we set up in fod_init(), free memory
 */
static
fod_deinit(struct pset *ps)
{
#ifdef DEBUG
	/*
	 * We only allow read-only views of files
	 */
	{
		int x;
		struct perpage *pp;

		for (x = 0; x < ps->p_len; ++x) {
			pp = find_pp(ps, x);
			ASSERT((pp->pp_flags & PP_M) == 0,
				"fod_deinit: dirty");
		}
	}
#endif
	return(pset_deinit(ps));
}

/*
 * fod_fillslot()
 *	Fill pset slot from a port
 */
static
fod_fillslot(struct pset *ps, struct perpage *pp, uint idx)
{
	uint pg;

	ASSERT_DEBUG(!(pp->pp_flags & (PP_V|PP_BAD)),
		"fod_fillslot: valid");
	pg = alloc_page();
	set_core(pg, ps, idx);
	if (pageio(pg, ps->p_pr, ptob(idx+ps->p_off), NBPG, FS_ABSREAD)) {
		free_page(pg);
		return(1);
	}

	/*
	 * Fill in the new page's value
	 */
	pp->pp_flags |= PP_V;
	pp->pp_refs = 1;
	pp->pp_flags &= ~(PP_M|PP_R);
	pp->pp_pfn = pg;
	return(0);
}

/*
 * fod_writeslot()
 *	Write pset slot out to its underlying port
 *
 * We don't have coherent mapped files, so extremely unclear what
 * this condition would mean.  Panic for now.
 */
static
fod_writeslot(struct pset *ps, struct perpage *pp, uint idx, voidfun iodone)
{
	ASSERT_DEBUG(pp->pp_flags & PP_V, "fod_writeslot: invalid");
	ASSERT(!(pp->pp_flags & PP_M), "fod_writeslot: dirty file");
	return(0);
}
@


1.2
log
@Rewrite of VM to get page reference counts right.
@
text
@d11 1
a11 1
#include <lib/alloc.h>
@


1.1
log
@Initial revision
@
text
@d11 1
d13 2
a14 5
extern void *malloc();

extern int pset_unref();
static int fod_fillslot(), fod_writeslot(), fod_init(), fod_deinit(),
	fod_unref();
d16 1
a16 1
	fod_deinit, pset_unref};
a20 5
 *
 * This would be really simple, except that we want to keep a pseudo-
 * view around so we can cache pages even when our COW reference breaks
 * away from us.  The hope is that the original view of a data page
 * will be desired multile times as the a.out is run again and again.
a24 13
	struct pview *pv;

	/*
	 * Get a pview, make it map 1:1 with our pset
	 */
	pv = ps->p_view = malloc(sizeof(struct pview));
	pv->p_set = ps;
	pv->p_vaddr = 0;
	pv->p_len = ps->p_len;
	pv->p_off = 0;
	pv->p_vas = 0;		/* Flags pseudo-view */
	pv->p_next = 0;
	pv->p_prot = PROT_RO;
d30 1
a30 1
 *	Tear down the goodies we set up in fod_init()
a34 1
	free(ps->p_view);
d36 13
a48 1
	ps->p_view = 0;
d50 1
a50 1
	return(0);
d65 1
d75 1
a75 1
	pp->pp_refs = 2;
a77 5

	/*
	 * Add our own internal reference to the page
	 */
	add_atl(pg, ps->p_view, ptob(idx));
@
