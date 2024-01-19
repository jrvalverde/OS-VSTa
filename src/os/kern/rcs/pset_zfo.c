head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.2
date	93.02.04.19.39.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.53.37;	author vandys;	state Exp;
branches;
next	;


desc
@Zero Fill On Demand pset handling
@


1.2
log
@Rewrite of VM to get page reference counts right.
@
text
@/*
 * pset_zfod.c
 *	Routines for implementing fill-on-demand (with zero) psets
 */
#include <sys/types.h>
#include <sys/pset.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/assert.h>

extern struct portref *swapdev;
extern int pset_writeslot(), pset_deinit();

static int zfod_fillslot(), zfod_init();
struct psetops psop_zfod =
	{zfod_fillslot, pset_writeslot, zfod_init, pset_deinit};

/*
 * zfod_init()
 *	Set up pset for zeroed memory
 */
static
zfod_init(struct pset *ps)
{
	return(0);
}

/*
 * zfod_fillslot()
 *	Fill pset slot with zeroes
 */
static
zfod_fillslot(struct pset *ps, struct perpage *pp, uint idx)
{
	uint pg;

	ASSERT_DEBUG(!(pp->pp_flags & (PP_V|PP_BAD)),
		"zfod_fillslot: valid");
	pg = alloc_page();
	set_core(pg, ps, idx);
	if (pp->pp_flags & PP_SWAPPED) {
		if (pageio(pg, swapdev, ptob(idx+ps->p_swapblk),
				NBPG, FS_ABSREAD)) {
			free_page(pg);
			return(1);
		}
	} else {
		bzero(ptov(ptob(pg)), NBPG);
	}

	/*
	 * Fill in the new page's value
	 */
	pp->pp_flags |= PP_V;
	pp->pp_flags &= ~(PP_M|PP_R);
	pp->pp_pfn = pg;
	pp->pp_refs = 1;
	return(0);
}
@


1.1
log
@Initial revision
@
text
@d12 1
a12 1
extern int pset_writeslot(), pset_unref();
d14 1
a14 1
static int zfod_fillslot(), zfod_init(), zfod_deinit();
d16 1
a16 1
	{zfod_fillslot, pset_writeslot, zfod_init, zfod_deinit, pset_unref};
a28 10
 * zfod_deinit()
 *	Clean up--no action needed
 */
static
zfod_deinit(struct pset *ps)
{
	return(0);
}

/*
d40 1
@
