/*
 * malloc.c
 *	Power-of-two storage allocator
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/core.h>
#include <sys/assert.h>
#include <sys/mutex.h>
#include <sys/vm.h>
#define MALLOC_INTERNAL
#include <sys/malloc.h>

extern struct core *core;
extern void *alloc_pages();
extern uint alloc_page();

#ifdef DEBUG
/*
 * Value to name mapping, to help the kernel debugger print
 * things out nicely
 */
char *n_allocname[MALLOCTYPES] = {
	"MT_RMAP",
	"MT_EVENT",
	"MT_EXITGRP",
	"MT_EXITST",
	"MT_MSG",
	"MT_SYSMSG",
	"MT_PORT",
	"MT_PORTREF",
	"MT_PVIEW",
	"MT_PSET",
	"MT_PROC",
	"MT_THREAD",
	"MT_KSTACK",
	"MT_VAS",
	"MT_PERPAGE",
	"MT_QIO",
	"MT_SCHED",
	"MT_SEG",
	"MT_EVENTQ",
	"MT_L1PT",
	"MT_L2PT",
	"MT_PGRP",
	"MT_ATL",
	"MT_FPU",
};
#endif /* DEBUG */

/*
 * malloc()
 *	Allocate block of given size
 */
void *
malloc(uint size)
{
	int pg;
	struct bucket *b;
	struct page *page;
	struct freehead *f;

	/*
	 * For pages and larger, allocate memory in units of
	 * pages. Power-of-two allocations, so > half page
	 * also consumes a whole page.
	 */
	if (size > (NBPG/2)) {
		int pgs;
		void *mem;

		pgs = btorp(size);
		mem = alloc_pages(pgs);
		ASSERT_DEBUG(sizeof(struct page) <= sizeof(struct core),
			"malloc: size page > core");
		page = (struct page *)&core[btop(vtop(mem))].c_long;
		page->p_bucket = PGSHIFT+pgs;
		return(mem);
	}

	/*
	 * Otherwise allocate from one of our buckets
	 */

	/*
	 * Cap minimum size
	 */
	if (size < (1 << MIN_BUCKET)) {
		size = (1 << MIN_BUCKET);
		b = &buckets[MIN_BUCKET];
	} else {
		uint mask, bucket;

		/*
		 * Poor man's FFS
		 */
		bucket = PGSHIFT-1;
		mask = 1 << bucket;
		while (!(size & mask)) {
			bucket -= 1;
			mask >>= 1;
		}

		/*
		 * Round up as needed
		 */
		if (size & (mask-1)) {
			bucket += 1;
		}
		b = &buckets[bucket];
	}

	/*
	 * We now know what bucket to use.  If it appears to be
	 * empty, grab a page now.
	 */
	if (EMPTY(b)) {
		pg = alloc_page();
	} else {
		pg = 0;
	}

	/*
	 * Lock and look "for real"
	 */
	(void)p_lock(&b->b_lock, SPL0);
	if (EMPTY(b) && !pg) {
		/*
		 * Argh.  It ran out between our look and our lock.
		 */
		v_lock(&b->b_lock, SPL0);
		pg = alloc_page();
		p_lock(&b->b_lock, SPL0);
	}

	/*
	 * If there's no memory, parcel our page up into chunks
	 * of the appropriate size.
	 */
	if (EMPTY(b)) {
		char *p;
		int x;
		struct core *c;

		ASSERT_DEBUG(b->b_elems == 0, "malloc: bad list");
		ASSERT_DEBUG(pg, "malloc: no pg, no mem");

		/*
		 * Fill in per-page information, flag page as
		 * consumed for kernel memory.
		 */
		p = ptov(ptob(pg));
		c = &core[pg];
		pg = 0;
		c->c_flags |= C_SYS;
		page = (struct page *)&(c->c_long);
		page->p_bucket = b-buckets;
		page->p_out = 0;

		/*
		 * Parcel as many chunks as will fit out of the page
		 */
		for (x = 0; x < NBPG; x += b->b_size) {
			/*
			 * Doubly-linked list insertion
			 */
			f = (struct freehead *)(p+x);
			f->f_forw = b->b_mem.f_forw;
			f->f_back = &b->b_mem;
			b->b_mem.f_forw->f_back = f;
			b->b_mem.f_forw = f;

			/*
			 * Tally elements available under bucket
			 */
			b->b_elems += 1;
		}

		/*
		 * Update count of pages consumed by this bucket
		 */
		b->b_pages += 1;
	}

	/*
	 * Take a chunk
	 */
	f = b->b_mem.f_forw;
	b->b_mem.f_forw = f->f_forw;
	f->f_forw->f_back = &b->b_mem;
	b->b_elems -= 1;

	/*
	 * Update per-page count
	 */
	page = (struct page *)&core[btop(vtop(f))].c_long;
	page->p_out += 1;

	/*
	 * Release lock.  If we didn't need our allocated page, free it
	 */
	v_lock(&b->b_lock, SPL0);
	if (pg) {
		free_page(pg);
	}

	/*
	 * Return memory
	 */
	return(f);
}

#ifdef DEBUG
/*
 * Tally of use of types
 */
ulong n_alloc[MALLOCTYPES];

/*
 * _malloc()
 *	Allocate with type attribute
 */
void *
_malloc(uint size, uint type)
{
	ASSERT(type < MALLOCTYPES, "_malloc: bad type");
	ATOMIC_INCL(&n_alloc[type]);
	return(malloc(size));
}

/*
 * _free()
 *	Free with type attribute
 */
void
_free(void *ptr, uint type)
{
	ASSERT(type < MALLOCTYPES, "_free: bad type");
	ATOMIC_DECL(&n_alloc[type]);
	free(ptr);
	return;
}
#endif /* DEBUG */

/*
 * free()
 *	Free a malloc()'ed memory element
 */
void
free(void *mem)
{
	struct page *page;
	uint pg, x;
	struct freehead *f;
	struct bucket *b;
	char *p;

	/*
	 * Get page information
	 */
	page = (struct page *)&core[pg = btop(vtop(mem))].c_long;

	/*
	 * If a whole page or more, free directly
	 */
	if (page->p_bucket > PGSHIFT) {
		free_pages(mem, page->p_bucket-PGSHIFT);
		return;
	}

	/*
	 * Get bucket and lock
	 */
	b = &buckets[page->p_bucket];
	p_lock(&b->b_lock, SPL0);

#ifdef DEBUG
	/*
	 * Slow, but can catch truly horrible bugs.  See if
	 * this memory is being freed when already free.
	 */
	{
		struct freehead *fstart;

		fstart = &b->b_mem;
		f = b->b_mem.f_forw;
		do {
			ASSERT(f != mem, "free: already on list");
			f = f->f_forw;
		} while (f != fstart);
	}
#endif

	/*
	 * Free chunk to bucket
	 */
	f = mem;
	f->f_forw = b->b_mem.f_forw;
	f->f_back = &b->b_mem;
	b->b_mem.f_forw->f_back = f;
	b->b_mem.f_forw = f;
	b->b_elems += 1;

	/*
	 * Update count.  If all elements in page are free, take
	 * back the page.  We will never do this if it would leave
	 * the bucket empty.
	 */
	page->p_out -= 1;
	if ((page->p_out == 0) && (b->b_elems > (NBPG / b->b_size))) {
		char *p;

		/*
		 * Walk each virtual address contained in the page.
		 * As we built our free list doubly-linked, we can
		 * then do an in-place removal.
		 */
		p = (char *)((ulong)mem & ~(NBPG-1));
		for (x = 0; x < NBPG; x += b->b_size) {
			f = (struct freehead *)(p + x);
			f->f_back->f_forw = f->f_forw;
			f->f_forw->f_back = f->f_back;
			b->b_elems -= 1;
		}
		b->b_pages -= 1;
		v_lock(&b->b_lock, SPL0);
		free_page(pg);
		return;
	}

	v_lock(&b->b_lock, SPL0);
}

/*
 * init_malloc()
 *	Set up the circular links on each bucket
 */
void
init_malloc(void)
{
	int x;
	struct bucket *b;

	for (x = 0; x < PGSHIFT; ++x) {
		b = &buckets[x];
		b->b_mem.f_forw = b->b_mem.f_back = &b->b_mem;
		b->b_elems = 0;
		b->b_pages = 0;
		b->b_size = (1 << x);
		init_lock(&b->b_lock);
	}
}
