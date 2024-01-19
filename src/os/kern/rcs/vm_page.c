head	1.11;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.8
	V1_1:1.8
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.11
date	95.01.27.17.07.25;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.11.16.19.36.07;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.10.23.17.44.12;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.44.14;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.04.23.22.42.08;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.31.04.40.21;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.30.01.10.38;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.11.19.19.49;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.05.00.36.41;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.04.19.39.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.55.07;	author vandys;	state Exp;
branches;
next	;


desc
@Per-page handling
@


1.11
log
@Unneeded #include
@
text
@/*
 * vm_page.c
 *	Routines for organizing pages of memory
 */
#include <sys/types.h>
#include <sys/pset.h>
#include <sys/mutex.h>
#include <sys/thread.h>
#include <sys/assert.h>
#include <sys/core.h>
#include <sys/percpu.h>
#include <sys/param.h>
#include <sys/vm.h>
#include <rmap.h>

extern void kern_addtrans(), kern_deletetrans();
extern char *heapstart, *heap;
extern int bootpgs;

/*
 * Free list rooted here.  Since a free page can't be attached to
 * a pset, we abuse the attach list field for a different kind of
 * list.
 */
uint freemem, totalmem;
static struct core *freelist = 0;
static lock_t mem_lock;		/* Spinlock for freelist */
static sema_t mem_sema;		/* Semaphore to wait for memory */
struct core *core, *coreNCORE;	/* Base and end of core info */

/*
 * Virtual address pool and its mutexes
 */
extern struct rmap *vmap;	/* Resource map */
static lock_t vmap_lock;	/* Mutex for vmap */
static sema_t vmap_sema;	/* How to sleep on it when need more */

/*
 * Mutexes for struct core fields.  We multiplex over NC_SEMA of
 * them to minimize interference between manipulations of unrelated
 * entries.  NC_SEMA must be a power of two.
 */
#define NC_SEMA (16)
static sema_t core_semas[NC_SEMA];

/*
 * alloc_page()
 *	Allocate a single page, return its pfn
 */
uint
alloc_page(void)
{
	struct thread *t = curthread;
	struct core *c;

	p_lock(&mem_lock, SPL0);
	/*
	 * This is a classic "sleeping for memory"
	 * scenario.  Because our allocate and free primitives
	 * are in units of a page, this becomes a simple FIFO
	 * semaphore queue for memory.
	 */
	while (freemem == 0) {
		p_sema_v_lock(&mem_sema, PRIHI, &mem_lock);
		p_lock(&mem_lock, SPL0);
	}
	freemem -= 1;

	/*
	 * We have our page.  Take it from the free list, initialize
	 * its fields, and return it.
	 */
	c = freelist;
	freelist = c->c_free;
	v_lock(&mem_lock, SPL0);
	c->c_flags = C_ALLOC;
	c->c_free = 0;
	return(c-core);
}

/*
 * free_page()
 *	Free a page to the freelist
 */
void
free_page(uint pfn)
{
	struct core *c;

	c = core+pfn;
	ASSERT_DEBUG((c >= core) && (c < coreNCORE),
		"free_page: bad page");
	c->c_flags &= ~C_ALLOC;
#ifdef DEBUG
	/* XXX only works for uniprocessor without preemption */
	{ struct core *c2;
	  for (c2 = freelist; c2; c2 = c2->c_free) {
	    ASSERT(c2 != c, "free_page: already free");
	  }
	}
#endif
	ASSERT_DEBUG(!(c->c_flags & C_BAD), "free_page: C_BAD");
	p_lock(&mem_lock, SPL0);
	c->c_free = freelist;
	freelist = c;
	freemem += 1;
	if (blocked_sema(&mem_sema)) {
		v_sema(&mem_sema);
	}
	v_lock(&mem_lock, SPL0);
}

/*
 * init_page()
 *	Initialize page data structures
 *
 * Our machine-dependent code has filled in the data structures we
 * use to configure our portable data structures.  After this routine
 * completes, memory allocations stop coming from the bootup heap and
 * start coming through the malloc/alloc_page routines.
 */
void
init_page(void)
{
	int x;
	struct core *c;

	ASSERT_DEBUG(heap != 0, "page_init: heap unavailable");

	/*
	 * Carve out core entries for all of memory space to top
	 * of memory, including any holes.  If the holes are large,
	 * perhaps we could free the memory for unused ranges of
	 * the core array?
	 */
	core = (struct core *)heap;
	coreNCORE = core+bootpgs;
	heap = (char *)coreNCORE;
	bzero(core, bootpgs*sizeof(struct core));

	/*
	 * Fill low range with C_SYS for our kernel text/data/bss
	 * and heap.  This assumes the kernel is relocated around 0.
	 * Initial tasks lie between end and heapstart.
	 * Note that we're marking our boot tasks' pages C_SYS;
	 * this will keep them out of the free pool, and we'll
	 * update their state when we create the task data
	 * structures.
	 */
	for (x = 0; x < btorp(heap); ++x) {
		core[x].c_flags = C_SYS;
	}

	/*
	 * Mark holes between memory ranges as C_BAD
	 */
	for (x = 0; x < nmemsegs-1; ++x) {
		ulong y, top;
		struct memseg *m;

		m = &memsegs[x];
		y = btop((char *)(m->m_base) + m->m_len);
		top = btop(m[1].m_base);
		while (y < top) {
			core[y].c_flags = C_BAD;
			++y;
		}
	}

	/*
	 * Now walk all memory, and put free pages onto free list
	 */
	freemem = 0;
	for (c = core; c < coreNCORE; ++c) {
		if (c->c_flags & (C_SYS|C_BAD)) {
			continue;
		}
		c->c_free = freelist;
		freelist = c;
		freemem += 1;
	}
	totalmem = freemem;

	/*
	 * The virtual map itself is filled in by machine-dependent
	 * code.  But we take responsibility for the mutexes which
	 * organize it.
	 */
	init_lock(&vmap_lock);
	init_sema(&vmap_sema); set_sema(&vmap_sema, 0);

	/*
	 * Initialize each of the core_semas to allow one person through
	 */
	for (x = 0; x < NC_SEMA; ++x) {
		init_sema(&core_semas[x]);
	}
}

/*
 * alloc_vmap()
 *	Allocate some virtual pages, sleep as needed
 */
uint
alloc_vmap(uint npg)
{
	uint idx;

	for (;;) {
		p_lock(&vmap_lock, SPL0);
		idx = rmap_alloc(vmap, npg);
		if (idx)
			break;
		(void)p_sema_v_lock(&vmap_sema, PRIHI, &vmap_lock);
	}
	v_lock(&vmap_lock, SPL0);
	return(idx);
}

/*
 * free_vmap()
 *	Put some space back into the virtual map
 */
static void
free_vmap(uint idx, uint npg)
{
	p_lock(&vmap_lock, SPL0);
	rmap_free(vmap, idx, npg);
	v_lock(&vmap_lock, SPL0);
}

/*
 * alloc_pages()
 *	Allocate some virtually contiguous kernel memory
 */
void *
alloc_pages(uint npg)
{
	char *vaddr;
	int x;
	uint pg;

	vaddr = (char *)ptob(alloc_vmap(npg));
	for (x = 0; x < npg; ++x) {
		pg = alloc_page();
		core[pg].c_flags |= C_SYS;
		kern_addtrans(vaddr + ptob(x), pg);
	}
	return((void *)vaddr);
}

/*
 * free_pages()
 *	Free some virtually contiguous kernel memory
 */
void
free_pages(void *vaddr, uint npg)
{
	int x;
	uint pg;

	for (x = 0; x < npg; ++x) {
		pg = btop(vtop(vaddr+ptob(x)));
		free_page(pg);
#ifdef DEBUG
		/*
		 * This might help catch users of stale pointers
		 */
		kern_deletetrans(vaddr+ptob(x), pg);
#endif
	}
	free_vmap(btop(vaddr), npg);
}

/*
 * lock_page()
 *	Apply an activity lock to the physical page
 *
 * We will sleep if needed.
 */
void
lock_page(uint pfn)
{
	uint slot = pfn & (NC_SEMA-1);

	p_sema(&core_semas[slot], PRIHI);
}

/*
 * clock_page()
 *	Like lock_page, but return with error code if page busy
 *
 * We still spin for the core_lock; it should NEVER be held long.
 */
clock_page(uint pfn)
{
	uint slot = pfn & (NC_SEMA-1);

	if (cp_sema(&core_semas[slot])) {
		return(1);
	}
	return(0);
}

/*
 * unlock_page()
 *	Release an existing page lock
 */
void
unlock_page(uint pfn)
{
	uint slot = pfn & (NC_SEMA-1);

	v_sema(&core_semas[slot]);
}

/*
 * set_core()
 *	Set pset/index information on a core entry
 */
void
set_core(uint pfn, struct pset *ps, uint idx)
{
	struct core *c;

	ASSERT_DEBUG((pfn > 0) && (pfn < bootpgs), "set_core: bad index");
	lock_page(pfn);
	c = &core[pfn];
	c->c_pset = ps;
	c->c_psidx = idx;
	unlock_page(pfn);
}
@


1.10
log
@Tidy up semaphore count handling, add assertions.  Convert
atomic ops so routine matches data element size.
@
text
@a13 1
#include <sys/qio.h>
@


1.9
log
@Delete some dead code
@
text
@d191 1
a191 2
	init_sema(&vmap_sema);
	set_sema(&vmap_sema, 0);
@


1.8
log
@Source reorg
@
text
@a319 16
 * free_iodone()
 *	Free page on I/O completion
 */
static void
free_iodone(struct qio *q)
{
	struct perpage *pp;
	extern void swap_iodone();

	pp = q->q_pp;
	free_page(pp->pp_pfn);
	pp->pp_flags &= ~(PP_R|PP_M|PP_V);
	unlock_slot(q->q_pset, pp);
}

/*
@


1.7
log
@Get rid of debugger trap
@
text
@d15 1
a15 1
#include <lib/rmap.h>
@


1.6
log
@Bracket core field fiddling with lock/unlock of page
@
text
@a64 7
#ifdef DEBUG
		/*
		 * XXX for now.  Isn't "really" a problem, but often it's
		 * good to look about when things get this desperate.
		 */
		dbg_enter();
#endif
@


1.5
log
@Add correct flagging of C_ALLOC flag in core struct
@
text
@d352 1
d356 1
@


1.4
log
@Add a drop-out to debugger when we completely deplete the
free memory pool.
@
text
@d84 1
a84 1
	c->c_flags = 0;
d101 1
@


1.3
log
@Add some more assertions.  Some only appropriate for uni/non-RT
@
text
@d65 7
@


1.2
log
@Rewrite of VM to get page reference counts right.
@
text
@d92 11
d175 1
a175 1
		if (c->c_flags & (C_SYS|C_BAD))
d177 1
@


1.1
log
@Initial revision
@
text
@d19 1
d75 1
a75 1
	freelist = (struct core *)c->c_atl;
d78 1
a78 1
	c->c_atl = 0;
d93 1
a93 2
	ASSERT(c->c_atl == 0, "free_page: still attached");
	c->c_atl = (struct atl *)freelist;
d96 1
a96 1
	if (blocked_sema(&mem_sema))
d98 1
a115 1
	extern int bootpgs;
d166 1
a166 1
		c->c_atl = (struct atl *)freelist;
d323 2
a324 4
 * deref_slot()
 *	Decrement reference count on a page slot, free page on last ref
 *
 * This routine assumes that it is being called under a locked slot.
d327 1
a327 1
deref_slot(struct pset *ps, struct perpage *pp, uint idx)
d329 1
a329 2
	(*(ps->p_ops->psop_unrefslot))(ps, pp, idx);
}
d331 4
a334 10
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
@
