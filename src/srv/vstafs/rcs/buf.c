head	1.22;
access;
symbols
	V1_3_1:1.21
	V1_3:1.21
	V1_2:1.20
	V1_1:1.20;
locks; strict;
comment	@ * @;


1.22
date	94.11.28.04.19.58;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	94.02.28.22.06.05;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	93.10.23.20.20.25;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.10.19.02.34.28;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.10.18.19.05.11;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.10.17.19.27.40;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.10.15.22.54.43;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.10.15.22.41.13;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.10.06.21.44.18;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.09.27.23.10.32;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.09.27.18.26.54;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.09.18.17.33.48;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.09.15.02.10.37;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.09.11.19.05.27;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.08.31.00.39.01;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.30.19.17.25;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.29.22.26.48;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.18.48.31;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.28.14.15.25;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.27.13.42.18;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.08.59.04;	author vandys;	state Exp;
branches;
next	;


desc
@In-core buffering routines
@


1.22
log
@Add special handling for 1st sector versus whole buffer.
Helps for case of dealing with file info but not whole file
contents (e.g., ls -l and such)
@
text
@/*
 * buf.c
 *	Buffering for extent-based filesystem
 *
 * To optimize I/O, we do not used a fixed buffer scheme; instead, we create
 * buffers which match the extent which is about to be read in.  A buffer
 * is looked up by the sector which starts the currently allocated extent.
 * When a file's allocation is changed (truncation, deletion, etc.) the
 * buffers must be freed at that point.
 *
 * Each buffer is up to EXTSIZ sectors in size.
 */
#include "vstafs.h"
#include "buf.h"
#include <sys/assert.h>
#include <hash.h>
#include <std.h>
#ifdef DEBUG
#include <stdio.h>
static uint nbuf = 0;		/* Running tally of buffers */
#endif

static uint bufsize;		/* # sectors held in memory currently */
static struct hash *bufpool;	/* Hash daddr_t -> buf */
static struct llist allbufs;	/* Time-ordered list, for aging */

#ifdef DEBUG
/*
 * dump_bufpool()
 *	Show buffer pool
 */
static void
dump_bufpool(void)
{
	struct llist *l;

	for (l = LL_NEXT(&allbufs); l != &allbufs; l = LL_NEXT(l)) {
		struct buf *b;

		b = l->l_data;
		printf(" Start %ld len %d locks %d flags 0x%x\n",
			b->b_start, b->b_nsec, b->b_locks, b->b_flags);
	}
}
#endif

/*
 * free_buf()
 *	Release buffer storage, remove from hash
 */
static void
free_buf(struct buf *b)
{
	ASSERT_DEBUG(b->b_list, "free_buf: null b_list");
	ll_delete(b->b_list);
	hash_delete(bufpool, b->b_start);
	bufsize -= b->b_nsec;
	ASSERT_DEBUG(b->b_data, "free_buf: null b_data");
	free(b->b_data);
#ifdef DEBUG
	bzero(b, sizeof(struct buf));
	ASSERT(nbuf > 0, "free_buf: underflow");
	nbuf -= 1;
#endif
	free(b);
}

/*
 * age_buf()
 *	Find the next available buf header, flush and free it
 *
 * Since this is basically a paging algorithm, it can become arbitrarily
 * complex.  The algorithm here tries to be simple, yet somewhat fair.
 */
static void
age_buf(void)
{
	struct llist *l;
	struct buf *b;

	/*
	 * Pick the oldest buf which isn't locked.
	 */
	for (l = allbufs.l_back; l != &allbufs; l = l->l_back) {
		/*
		 * Only skip if wired
		 */
		b = l->l_data;
		if (b->b_locks) {
			continue;
		}

		/*
		 * Sync out data (sync_buf() checks for DIRTY)
		 */
		sync_buf(b);

		/*
		 * Remove from list, update data structures
		 */
		free_buf(b);
		return;
	}
#ifdef DEBUG
	printf("No buffers aged out of %d in pool:\n", bufsize);
	dump_bufpool();
#endif
	ASSERT(bufsize <= CORESEC, "age_buf: buffers too large");
}

#ifdef DEBUG
/*
 * check_span()
 *	Assert that a buf doesn't contain the given value
 */
static int
check_span(long key, void *data, void *arg)
{
	daddr_t d;
	struct buf *b;

	d = (daddr_t)arg;
	b = data;
	if ((d >= key) && (d < (key + b->b_nsec))) {
		fprintf(stderr, "overlap: %ld resides in %ld len %d\n",
			d, key, b->b_nsec);
		ASSERT(0, "check_span: overlap");
	}
	return(0);
}
#endif /* DEBUG */

/*
 * find_buf()
 *	Given starting sector #, return pointer to buf
 */
struct buf *
find_buf(daddr_t d, uint nsec)
{
	struct buf *b;

	ASSERT_DEBUG(nsec > 0, "find_buf: zero");
	ASSERT_DEBUG(nsec <= EXTSIZ, "find_buf: too big");

	/*
	 * If we can find it, this is easy
	 */
	b = hash_lookup(bufpool, d);
	if (b) {
		return(b);
	}

#ifdef DEBUG
	/*
	 * Make sure there isn't a block which spans it
	 */
	hash_foreach(bufpool, check_span, (void *)d);
#endif

	/*
	 * Get a buf struct
	 */
	b = malloc(sizeof(struct buf));
	if (b == 0) {
		return(0);
	}

	/*
	 * Make room in our buffer cache if needed
	 */
	while ((bufsize+nsec) > CORESEC) {
		age_buf();
	}

	/*
	 * Get the buffer space
	 */
	b->b_data = malloc(stob(nsec));
	if (b->b_data == 0) {
		free(b);
		return(0);
	}

	/*
	 * Add us to pool, and mark us very new
	 */
	b->b_list = ll_insert(&allbufs, b);
	if (b->b_list == 0) {
		free(b->b_data);
		free(b);
		return(0);
	}
	if (hash_insert(bufpool, d, b)) {
		ll_delete(b->b_list);
		free(b->b_data);
		free(b);
		return(0);
	}

	/*
	 * Fill in the rest & return
	 */
	b->b_start = d;
	b->b_nsec = nsec;
	b->b_flags = 0;
	b->b_locks = 0;
	bufsize += nsec;
#ifdef DEBUG
	nbuf += 1;
	ASSERT(nbuf <= CORESEC, "find_buf: too many");
#endif
	return(b);
}

/*
 * resize_buf()
 *	Indicate that the cached region is changing to newsize
 *
 * If "fill" is non-zero, the incremental contents are filled from disk.
 * Otherwise the buffer space is left uninitialized.
 *
 * Returns 0 on success, 1 on error.
 */
int
resize_buf(daddr_t d, uint newsize, int fill)
{
	char *p;
	struct buf *b;

	ASSERT_DEBUG(newsize <= EXTSIZ, "resize_buf: too large");
	ASSERT_DEBUG(newsize > 0, "resize_buf: zero");
	/*
	 * If it isn't currently buffered, we don't care yet
	 */
	if (!(b = hash_lookup(bufpool, d))) {
		return(0);
	}
#ifdef DEBUG
	/* This isn't fool-proof, but should catch most transgressions */
	if (newsize > b->b_nsec) {
		hash_foreach(bufpool, check_span,
			(void *)(b->b_start + newsize - 1));
	}
#endif

	/*
	 * Current activity--move to end of age list
	 */
	ll_movehead(&allbufs, b->b_list);

	/*
	 * Resize to current size is a no-op
	 */
	if (newsize == b->b_nsec) {
		return(0);
	}

	/*
	 * Get the buffer space
	 */
	ASSERT_DEBUG(!(fill && (newsize < b->b_nsec)),
		"resize_buf: fill && shrink");
	p = realloc(b->b_data, stob(newsize));
	if (p == 0) {
		return(1);
	}
	b->b_data = p;

	/*
	 * If needed, fill from disk
	 */
	if (fill) {
		read_secs(b->b_start + b->b_nsec, p + stob(b->b_nsec),
			newsize - b->b_nsec);
	}

	/*
	 * Update buf and return success
	 */
	bufsize = (int)bufsize + ((int)newsize - (int)b->b_nsec);
	b->b_nsec = newsize;
	while (bufsize > CORESEC) {
		age_buf();
	}
	return(0);
}

/*
 * index_buf()
 *	Get a pointer to a run of data under a particular buf entry
 *
 * As a side effect, move us to front of list to make us relatively
 * undesirable for aging.
 */
void *
index_buf(struct buf *b, uint index, uint nsec)
{
	ASSERT((index+nsec) <= b->b_nsec, "index_buf: too far");

	ll_movehead(&allbufs, b->b_list);
	if ((index == 0) && (nsec == 1)) {
		/*
		 * Only looking at 1st sector.  See about reading
		 * only 1st sector, if we don't yet have it.
		 */
		if ((b->b_flags & B_SEC0) == 0) {
			/*
			 * Load the sector, mark it as present
			 */
			read_secs(b->b_start, b->b_data, 1);
			b->b_flags |= B_SEC0;
		}
	} else if ((b->b_flags & B_SECS) == 0) {
		/*
		 * Otherwise if we don't have the whole buffer, get
		 * it now.
		 */
		read_secs(b->b_start, b->b_data, b->b_nsec);
		b->b_flags |= (B_SEC0|B_SECS);
	}
	return((char *)b->b_data + stob(index));
}

/*
 * init_buf()
 *	Initialize the buffering system
 */
void
init_buf(void)
{
	ll_init(&allbufs);
	bufpool = hash_alloc(CORESEC/8);
	bufsize = 0;
	ASSERT(bufpool, "init_buf: bufpool");
}

/*
 * dirty_buf()
 *	Mark the given buffer dirty
 */
void
dirty_buf(struct buf *b)
{
	b->b_flags |= B_DIRTY;
}

/*
 * lock_buf()
 *	Lock down the buf; make sure it won't go away
 */
void
lock_buf(struct buf *b)
{
	b->b_locks += 1;
	ASSERT(b->b_locks > 0, "lock_buf: overflow");
}

/*
 * unlock_buf()
 *	Release previously taken lock
 */
void
unlock_buf(struct buf *b)
{
	ASSERT(b->b_locks > 0, "unlock_buf: underflow");
	b->b_locks -= 1;
}

/*
 * sync_buf()
 *	Sync back buffer if dirty
 *
 * Write back the 1st sector, or the whole buffer, as appropriate
 */
void
sync_buf(struct buf *b)
{
	ASSERT_DEBUG(b->b_flags & (B_SEC0 | B_SECS), "sync_buf: not ref'ed");

	/*
	 * Skip it if not dirty
	 */
	if (!(b->b_flags & B_DIRTY)) {
		return;
	}

	/*
	 * Do the I/O--whole buffer, or just 1st sector if that was
	 * the only sector referenced.
	 */
	if (b->b_flags & B_SECS) {
		write_secs(b->b_start, b->b_data, b->b_nsec);
	} else {
		write_secs(b->b_start, b->b_data, 1);
	}
	b->b_flags &= ~B_DIRTY;
}

/*
 * inval_buf()
 *	Clear out (without sync'ing) some buffer data
 *
 * This routine will handle multiple buffer entries, but "d" must
 * point to an aligned beginning of such an entry.
 */
void
inval_buf(daddr_t d, uint len)
{
	struct buf *b;

	for (;;) {
		b = hash_lookup(bufpool, d);
		if (b) {
			free_buf(b);
		}
		if (len <= EXTSIZ) {
			break;
		}
		d += EXTSIZ;
		len -= EXTSIZ;
	}
}

/*
 * sync()
 *	Write all dirty buffers to disk
 */
void
sync(void)
{
	struct llist *l;

	for (l = LL_NEXT(&allbufs); l != &allbufs; l = LL_NEXT(l)) {
		struct buf *b = l->l_data;

		if (b->b_flags & B_DIRTY) {
			sync_buf(b);
		}
	}
}
@


1.21
log
@Convert to syslog()
@
text
@a206 1
	read_secs(d, b->b_data, nsec);
d301 20
d373 1
a373 5
 * XXX we sync the whole thing, with the hope that usually a reasonable
 * fraction of the extent will all be dirty if any part is dirty.  If
 * not, we would have to add a bitmask marking which sectors are dirty,
 * and only write the appropriate ones.  Keep it simple until we find
 * it must be complicated.
d378 2
d388 2
a389 1
	 * Do the I/O
d391 5
a395 1
	write_secs(b->b_start, b->b_data, b->b_nsec);
@


1.20
log
@Source reorg
@
text
@a20 1
#undef TRACE /* Very noisy */
a53 3
#ifdef TRACE
	printf("free_buf start %ld len %d\n", b->b_start, b->b_nsec);
#endif
a230 3
#ifdef TRACE
	printf("resize_buf start %ld len %d fill %d\n", d, newsize, fill);
#endif
a244 3
#ifdef TRACE
	printf(" oldsize %d\n", b->b_nsec);
#endif
a324 3
#ifdef TRACE
	printf("dirty_buf start %ld\n", b->b_start);
#endif
a362 4
#ifdef TRACE
	printf("sync_buf start %ld (%s)\n", b->b_start,
		(b->b_flags & B_DIRTY) ? "dirty" : "clean");
#endif
a388 3
#ifdef TRACE
	printf("inval_buf start %ld len %d\n", d, len);
#endif
a391 4
#ifdef TRACE
			printf(" extent at %ld len %d\n", b->b_start,
				b->b_nsec);
#endif
a410 3
#ifdef TRACE
	printf("sync\n");
#endif
a414 3
#ifdef TRACE
			printf(" write %ld len %d\n", b->b_start, b->b_nsec);
#endif
@


1.19
log
@Add instrumentation to perhaps catch buffer leaks
@
text
@d13 2
a14 2
#include <vstafs/vstafs.h>
#include <vstafs/buf.h>
d16 1
a16 2
#include <lib/llist.h>
#include <lib/hash.h>
@


1.18
log
@For buffer dump, walk list forward, not backwards
@
text
@d21 1
d67 2
d214 4
@


1.17
log
@Fix missing tally of outstanding buffer space.  Also add
debug routine for dumping current buffers.
@
text
@d37 3
a39 1
	struct buf *b;
a40 1
	for (l = allbufs.l_back; l != &allbufs; l = l->l_back) {
d187 1
a187 1
	 * Add us to pool
@


1.16
log
@Add some debug instrumentation
@
text
@d28 19
d105 4
d209 1
@


1.15
log
@Update count of size of buffer pool as we resize buffers.
@
text
@d38 1
d42 1
d44 3
@


1.14
log
@Add tracing printf()'s, useful when debugging
@
text
@d21 1
a21 1
/* #define TRACE /* Very noisy */
d81 1
d222 5
d255 1
d257 3
@


1.13
log
@Check special case of resizing to same size--used to grab
the fs_file from scratch.
@
text
@d21 1
d35 3
d198 3
d215 3
d289 3
d330 4
d360 3
d366 4
d389 3
d396 3
@


1.12
log
@Don't check overlap on shrink; not an issue
@
text
@d211 7
@


1.11
log
@Re-order sanity checks
@
text
@d195 1
d204 4
a207 1
	hash_foreach(bufpool, check_span, (void *)(b->b_start + newsize - 1));
@


1.10
log
@Fix DEBUG, update buffer length on resize
@
text
@d19 3
d92 5
a96 1
	ASSERT((d < key) || (d >= (key + b->b_nsec)), "check_span: overlap");
@


1.9
log
@Forgot to initialize b_locks
@
text
@d187 1
a187 6
#ifdef DEBUG
	/* This isn't fool-proof, but should catch most transgressions */
	ASSERT(newsize <= MAXEXTSIZ, "resize_buf: too large");
	ASSERT(((ulong)d % EXTSIZ) == 0, "resize_buf: misaligned");
	hash_foreach(bufpool, check_span, (void *)(b->b_start + newsize - 1));
#endif
d194 4
d208 1
@


1.8
log
@Handle multiple extents within routine
@
text
@d167 1
@


1.7
log
@Add sync() to flush all dirty blocks
@
text
@d172 2
a173 2
 * extend_buf()
 *	Indicate that the cached region is growing to newsize
d181 1
a181 1
extend_buf(daddr_t d, uint newsize, int fill)
d188 2
a189 1
	ASSERT(newsize <= MAXEXTSIZ, "extend_buf: too large");
d202 2
d315 3
d324 10
a333 3
	b = hash_lookup(bufpool, d);
	if (b) {
		free_buf(b);
@


1.6
log
@Incorrect range check for unlock
@
text
@d323 18
@


1.5
log
@Clean up -Wall warnings
@
text
@d278 1
a278 1
	ASSERT(b->b_locks > 1, "unlock_buf: underflow");
@


1.4
log
@Allow locking to next
@
text
@d109 2
a110 1
	if (b = hash_lookup(bufpool, d)) {
@


1.3
log
@Further file creation/deletion stuff
@
text
@d56 1
a56 1
		 * Only skip if BUSY
d59 1
a59 1
		if (b->b_flags & B_BUSY) {
d266 2
a267 2
	ASSERT(!(b->b_flags & B_BUSY), "lock_buf: locked");
	b->b_flags |= B_BUSY;
d277 2
a278 2
	ASSERT(b->b_flags & B_BUSY, "unlock_buf: unlocked");
	b->b_flags &= ~B_BUSY;
@


1.2
log
@Convert buffer resize to use only block addresses.  Do rest of
read/write support.
@
text
@d25 14
d71 1
a71 5
		ll_delete(l);
		hash_delete(bufpool, b->b_start);
		bufsize -= b->b_nsec;
		free(b->b_data);
		free(b);
d306 15
@


1.1
log
@Initial revision
@
text
@d170 1
a170 1
extend_buf(struct buf *b, ulong newsize, int fill)
d173 1
d180 7
@
