head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.4
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	94.04.19.00.27.10;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.03.04.02.01.36;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.03.20.15.39;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.53.48;	author vandys;	state Exp;
branches;
next	;


desc
@Page View handling
@


1.5
log
@Convert to MALLOC
@
text
@/*
 * pview.c
 *	Routines for creating/deleting pviews
 */
#include <sys/vas.h>
#include <sys/pview.h>
#include <sys/pset.h>
#include <sys/malloc.h>
#include <sys/assert.h>
#include <std.h>

/*
 * alloc_pview()
 *	Create a pview in terms of the given pset
 */
struct pview *
alloc_pview(struct pset *ps)
{
	struct pview *pv;

	pv = MALLOC(sizeof(struct pview), MT_PVIEW);
	bzero(pv, sizeof(struct pview));
	pv->p_set = ps;
	ref_pset(ps);
	pv->p_len = ps->p_len;
	return(pv);
}

/*
 * free_pview()
 *	Delete view, remove reference to pset
 */
void
free_pview(struct pview *pv)
{
	deref_pset(pv->p_set);
	FREE(pv, MT_PVIEW);
}

/*
 * dup_view()
 *	Duplicate an existing view
 */
struct pview *
dup_pview(struct pview *opv)
{
	struct pview *pv;

	pv = MALLOC(sizeof(struct pview), MT_PVIEW);
	bcopy(opv, pv, sizeof(*pv));
	ref_pset(pv->p_set);
	pv->p_next = 0;
	pv->p_vas = 0;
	return(pv);
}

/*
 * copy_view()
 *	Make a copy of the underlying set, create a view to it
 */
struct pview *
copy_pview(struct pview *opv)
{
	struct pset *ps;
	struct pview *pv;

	pv = dup_pview(opv);
	ps = copy_pset(opv->p_set);
	deref_pset(opv->p_set);
	pv->p_set = ps;
	ref_pset(ps);
	return(pv);
}

/*
 * remove_pview()
 *	Given vaddr, remove corresponding pview
 *
 * Frees pview after deref'ing the associated pset
 */
void
remove_pview(struct vas *vas, void *vaddr)
{
	struct pview *pv;
	extern struct pview *detach_pview();

	pv = detach_pview(vas, vaddr);
	deref_pset(pv->p_set);
	FREE(pv, MT_PVIEW);
}
@


1.4
log
@Fix -Wall warnings
@
text
@d8 1
a9 1
#include <alloc.h>
d21 1
a21 1
	pv = malloc(sizeof(struct pview));
d37 1
a37 1
	free(pv);
d49 1
a49 1
	pv = malloc(sizeof(struct pview));
d89 1
a89 1
	free(pv);
@


1.3
log
@Source reorg
@
text
@d10 1
@


1.2
log
@Fix up referencing of pset while copying a pset
@
text
@d9 1
a9 1
#include <lib/alloc.h>
@


1.1
log
@Initial revision
@
text
@d68 1
d70 1
@
