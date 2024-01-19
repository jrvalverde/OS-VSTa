head	1.7;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.6
	V1_1:1.6
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.7
date	94.02.07.19.40.06;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.10.23.20.18.51;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.10.17.19.25.28;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.09.27.23.07.44;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.03.00.00.16;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.16.22.22.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.59.37;	author vandys;	state Exp;
branches;
next	;


desc
@User-memory allocator, based on same concepts as kernel one, but
significan modifications to map into a user environment.
@


1.7
log
@Add space in units of the bucket's size, not the currently
requested size.
@
text
@/*
 * malloc.c
 *	Routines for allocating/freeing memory
 *
 * This routine is adapted from the power-of-two allocator used
 * in the kernel.  The biggest trick is how to keep the per-virtual-page
 * information when there's no underlying per-page array.  We do it
 * here by allocation in chunks starting at MINPG and doubling to a
 * max of MAXPG.  Each chunk describes its range on a per-page basis.
 *
 * Unlike the kernel allocator, we don't bother trying to pull
 * unused pages out of the allocator.  We assume the VM system will
 * get its physical pages back as needed; we thus save the trouble
 * of maintaining doubly-linked lists.
 */
#include <sys/param.h>
#include <sys/mman.h>
#include <std.h>

/*
 * A poor man's <assert.h> to avoid circular library dependencies
 */
#ifdef DEBUG
#define ASSERT(cond, msg) if (!(cond)) \
	{ write(2, msg, sizeof(msg)-1); abort(); }
#else
#define ASSERT(cond, msg) /* Nothing */
#endif

#define MINPG (16)		/* Smallest storage allocation size */
#define MAXPG (256)		/*  ...largest */
#define MAXCHUNK (16)		/* Max chunks: supports > 12 Mb memory */

/*
 * One of these exists for each chunk of pages
 */
struct chunk {
	void *c_vaddr;		/* Base of mem for chunk */
	int c_len;		/* # pages in chunk */
	ushort *c_perpage;	/* Per-page information (1st page of chunk) */
	struct chunk		/* List of chunks */
		*c_next;
};
static struct chunk *chunks = 0;

/*
 * Our per-storage-size information
 */
#define MIN_BUCKET 4		/* At least 16 bytes allocated */
#define MAX_BUCKET 15		/* Up to 32k chunks */
struct bucket {
	void *b_mem;		/* Next chunk in bucket */
	uint b_elems;		/* # chunks available in this bucket */
	uint b_pages;		/* # pages used for this bucket size */
	uint b_size;		/* Size of this kind of chunk */
} buckets[MAX_BUCKET+1];

/*
 * init_malloc()
 *	Set up each bucket
 */
static void
init_malloc(void)
{
	int x;

	for (x = 0; x <= MAX_BUCKET; ++x) {
		buckets[x].b_size = (1 << x);
	}
}

/*
 * free_core()
 *	Free core from an alloc_core()
 *
 * Only used for "large" allocations; bucket allocations are always
 * left in the bucket and never freed to the OS (except on exit!).
 */
static void
free_core(void *mem)
{
	struct chunk *c, **cp;

	/*
	 * Patch this chunk out of the list
	 */
	cp = &chunks;
	mem = (char *)mem - NBPG;
	for (c = chunks; c; c = c->c_next) {
		if (c == mem) {
			*cp = c->c_next;
			break;
		}
		cp = &c->c_next;
	}

	/*
	 * Dump the memory back to the operating system
	 */
	munmap(mem, ptob(c->c_len + 1));
}

/*
 * alloc_core()
 *	Get more core from the OS, add it to our pool
 */
static void *
alloc_core(uint pgs)
{
	void *newmem;
	struct chunk *c;

	/*
	 * Get new virtual space from the OS
	 */
	newmem = mmap(0, ptob(pgs+1), PROT_READ|PROT_WRITE,
		MAP_ANON, 0, 0);
	if (!newmem) {
		return(0);
	}

	/*
	 * First page is for our chunk description
	 */
	c = newmem;
	newmem = (char *)newmem + NBPG;
	c->c_vaddr = newmem;
	c->c_len = pgs;
	c->c_perpage = (ushort *)((char *)c + sizeof(struct chunk));
	c->c_next = chunks;
	chunks = c;
	bzero(c->c_perpage, sizeof(ushort *) * c->c_len);

	return(newmem);
}

/*
 * alloc_pages()
 *	Get some memory from the pool
 */
static void *
alloc_pages(uint pgs)
{
	static char *freemem = 0;
	static int freepgs = 0;
	static int allocsz = MINPG;

retry:
	/*
	 * If we have enough here, let them have it
	 */
	if (freepgs >= pgs) {
		void *p;

		p = freemem;
		freemem += ptob(pgs);
		freepgs -= pgs;
		return(p);
	}

	/*
	 * Sanity
	 */
	ASSERT(allocsz >= pgs, "malloc: alloc_pages: too big");

	/*
	 * Get more core.  Note that there might be some residual
	 * memory in freemem; we lose it.
	 */
 	freemem = alloc_core(allocsz);
	if (freemem == 0) {
		return(0);
	}
	freepgs = allocsz;

	/*
	 * Bump allocation size until we reach max
	 */
	if (allocsz < MAXPG) {
		allocsz <<= 1;
	}
	goto retry;
}

/*
 * find_chunk()
 *	Scan all chunks for the one describing this memory
 */
static struct chunk *
find_chunk(void *mem)
{
	struct chunk *c;

	for (c = chunks; c; c = c->c_next) {
		if ((c->c_vaddr <= mem) &&
			(((char *)c->c_vaddr + ptob(c->c_len)) >
			 (char *)mem)) {
				break;
		}
	}
	ASSERT(c, "malloc: find_chunk: no chunk");
	return(c);
}

/*
 * set_size()
 *	Set size attribute for a given set of pages
 */
static void
set_size(void *mem, int size)
{
	struct chunk *c;
	int idx;

	/*
	 * Find chunk containing address
	 */
	c = find_chunk(mem);

	/*
	 * Get index for perpage slot.  Set size attribute for each slot.
	 */
 	idx = btop((char *)mem - (char *)(c->c_vaddr));
	ASSERT(size < 32768, "set_size: overflow");
	c->c_perpage[idx] = size;
}

/*
 * get_size()
 *	Get size attribute for some memory
 */
static
get_size(void *mem)
{
	struct chunk *c;
	int idx;

	c = find_chunk(mem);
 	idx = btop((char *)mem - (char *)(c->c_vaddr));
	return(c->c_perpage[idx]);
}

/*
 * malloc()
 *	Allocate block of given size
 */
void *
malloc(unsigned int size)
{
	struct bucket *b;
	void *f;
	static int init = 0;

	/*
	 * Sorta lame
	 */
 	if (!init) {
		init_malloc();
		init = 1;
	}

	/*
	 * For pages and larger, allocate memory in units of
	 * pages.
	 */
	if (size > (1 << MAX_BUCKET)) {
		uint pgs;
		void *mem;

		pgs = btorp(size);
		mem = alloc_core(pgs);
		set_size(mem, pgs + MAX_BUCKET);
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
		bucket = MAX_BUCKET;
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
	 * We now know what bucket to use.  Add memory if needed.
	 */
	if (!(b->b_mem)) {
		char *p;
		uint x, pgs;

		/*
		 * Fill in per-page information
		 */
		pgs = btorp(b->b_size);
		p = alloc_pages(pgs);
		set_size(p, b-buckets);

		/*
		 * Parcel as many chunks as will fit out of the memory
		 */
		for (x = 0; x < ptob(pgs); x += b->b_size) {
			/*
			 * Add to list
			 */
			*(void **)(p + x) = b->b_mem;
			b->b_mem = (void *)(p + x);

			/*
			 * Tally elements available under bucket
			 */
			b->b_elems += 1;
		}

		/*
		 * Update count of pages consumed by this bucket
		 */
		b->b_pages += pgs;
	}

	/*
	 * Take a chunk
	 */
	f = b->b_mem;
	b->b_mem = *(void **)f;

	/*
	 * Return memory
	 */
	return(f);
}

/*
 * free()
 *	Free a malloc()'ed memory element
 */
void
free(void *mem)
{
	struct bucket *b;
	ushort sz;

	/*
	 * Ignore NULL
	 */
	if (mem == 0) {
		return;
	}

	/*
	 * Get allocation information for this data element
	 */
	sz = get_size(mem);

	/*
	 * If a whole page or more, free directly
	 */
	if (sz > MAX_BUCKET) {
		free_core(mem);
		return;
	}

	/*
	 * Get bucket
	 */
	b = &buckets[sz];

	/*
	 * Free chunk to bucket
	 */
	*(void **)mem = b->b_mem;
	b->b_mem = mem;
	b->b_elems += 1;
}

/*
 * realloc()
 *	Grow size of existing memory block
 *
 * We only "grow" it if the new size is within the same power of
 * two.  Otherwise we allocate a new block and copy over.
 */
void *
realloc(void *mem, uint newsize)
{
	uint oldsize;
	int copy = 0;
	void *newmem;

	/*
	 * When the old pointer is 0, this is the special case of the
	 * first use of the pointer.  It spares the user code from having
	 * to special-case the first time an element is allocated.
	 */
	if (mem == 0) {
		return(malloc(newsize));
	}

	/*
	 * Find out how big it is now
	 */
	oldsize = get_size(mem);
	if (oldsize <= MAX_BUCKET) {
		oldsize = (1 << oldsize);
	} else {
		oldsize = ptob(oldsize - MAX_BUCKET);
	}

	/*
	 * If shrinking or same size no problem.  The recorded size is the
	 * actual size of the allocated block--so growth within the same
	 * power of two (for small) or page count (for large) will succeed
	 * here with great efficiency.
	 *
	 * If the size is "large" (> MAX_BUCKET) and the new size is half
	 * as small or smaller, actually convert to a smaller amount of
	 * memory.  Large buffers are munmap()'ed, which will free core
	 * back to the system.
	 */
 	if (newsize <= oldsize) {
		if ((oldsize > (1 << MAX_BUCKET)) &&
				(newsize <= (oldsize >> 1))) {
			/* Fall into below for realloc */ ;
		} else {
			return(mem);
		}
	}

	/*
	 * Otherwise allocate a new block, copy over the old data, and
	 * free the old block.
	 */
	newmem = malloc(newsize);
	if (newmem == 0) {
		return(0);
	}
	bcopy(mem, newmem, (oldsize < newsize) ? oldsize : newsize);
	free(mem);
	return(newmem);
}

/*
 * calloc()
 *	Return cleared space
 */
void *
calloc(unsigned int nelem, unsigned int elemsize)
{
	void *p;
	unsigned int total = nelem*elemsize;

	/*
	 * Get space, bail if can't alloc
	 */
	p = malloc(total);
	if (p == 0) {
		return(0);
	}

	/*
	 * Clear it, return a pointer
	 */
	bzero(p, total);
	return(p);
}
@


1.6
log
@When reallocating large (> MAX_BUCKET) memory areas, hand the
old buffer back to the system if we're going to be using less
than half.
@
text
@d318 1
a318 1
		pgs = btorp(size);
@


1.5
log
@Incorrect patch-out because we didn't advance our back-pointer pointer
@
text
@d437 5
d444 6
a449 1
		return(mem);
d460 1
a460 1
	bcopy(mem, newmem, oldsize);
@


1.4
log
@Overflow of chunk size when > 64K.  Changed to encode > NBPG as
count of pages.
@
text
@d94 1
@


1.3
log
@Add calloc()
@
text
@d200 1
a200 1
	ASSERT(c, "malloc: set_size: no chunk");
d223 1
d271 1
a271 1
		set_size(mem, ptob(pgs));
d427 2
d446 3
@


1.2
log
@Ignore NULL pointers to free()
@
text
@d447 25
@


1.1
log
@Initial revision
@
text
@d365 7
@
