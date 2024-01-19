/*
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
