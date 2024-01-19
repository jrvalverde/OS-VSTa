head	1.7;
access;
symbols
	V1_3_1:1.6
	V1_3:1.5
	V1_2:1.3
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.7
date	94.06.16.17.27.40;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.04.26.21.39.03;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.03.23.21.57.04;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.02.02.19.57.55;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.12.22.00.22.27;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.11.16.02.48.09;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Block buffer
@


1.7
log
@Remove unused variable
@
text
@/*
 * block.c
 *	Routines for caching blocks out of our filesystem
 */
#include "dos.h"
#include <llist.h>
#include <sys/assert.h>
#include <std.h>
#include <fcntl.h>

static int cached = 0;		/* # blocks currently cached */

/*
 * Structure describing a block in the cache
 */
struct block {
	int b_blkno;			/* Block we describe */
	int b_flags;			/* Flags (see below) */
	void *b_data;			/* Pointer to data */
	struct llist *b_all,		/* Circular list of all blocks */
		*b_hash;		/* Linked list on hash collision */
	int b_refs;			/* # active references */
};

/*
 * Bits in b_flags
 */
#define B_DIRTY 1		/* New data in buffer */

/*
 * Head of hash chains for each hash value, plus linked list of
 * all blocks outstanding.
 */
#define HASHSIZE 0x20
#define HASHMASK (HASHSIZE-1)
static struct llist hash[HASHSIZE];
static struct llist allblocks;

/*
 * Mapping between cluster numbers and underlying byte offsets
 */
#define BOFF(clnum) (((clnum)-2)*BLOCKSIZE + data0)
extern ulong data0;	/* Offset to start of blocks on dev */

/*
 * bnew()
 *	Get a new block, associate it with the named block number
 *
 * Since we're only allowed NCACHE blocks in-core, we might have to
 * push a dirty block out.  For a boot filesystem, we just push
 * synchronously.  Other filesystems will doubtless prefer to push
 * dirty blocks asynchronously while still scanning forward for
 * a clean one.
 *
 * On return, the new block has a single reference.
 */
static struct block *
bnew(int blkno)
{
	struct block *b;
	struct llist *l;

	/*
	 * Not above our limit--easy living!
	 */
	if (cached < NCACHE) {
		b = malloc(sizeof(struct block));
		if (b) {
			b->b_data = malloc(BLOCKSIZE);
			if (b->b_data) {
				b->b_blkno = blkno;
				b->b_flags = 0;
				b->b_all = ll_insert(&allblocks, b);
				b->b_hash =
					ll_insert(&hash[blkno & HASHMASK], b);
				b->b_refs = 1;
				cached += 1;
				return(b);
			}
			free(b);
		}
	}
	ASSERT(cached > 0, "bnew: no memory");

	/*
	 * Either above our limit, or we couldn't get any more memory
	 */

	/*
	 * Scan all blocks.  When we find something, move our
	 * placeholder up to one past that point.
	 */
	for (l = allblocks.l_forw; l != &allblocks; l = l->l_forw) {
		/*
		 * Ignore if reference is held
		 */
		b = l->l_data;
		if (b->b_refs > 0)
			continue;

		/*
		 * Clean it if dirty
		 */
		if (b->b_flags & B_DIRTY) {
			lseek(blkdev, BOFF(b->b_blkno), 0);
			write(blkdev, b->b_data, BLOCKSIZE);
			b->b_flags &= ~B_DIRTY;
		}

		/*
		 * Move us to the "newest" position
		 */
		ll_delete(b->b_all);
		b->b_all = ll_insert(&allblocks, b);
		ASSERT_DEBUG(b->b_all, "bnew: lost llist");

		/*
		 * Update the hash chain it resides on, and return it.
		 * Our caller is responsible for putting the right
		 * data into the buffer.
		 */
		ll_delete(b->b_hash);
		b->b_hash = ll_insert(&hash[blkno & HASHMASK], b);
		b->b_refs = 1;
		b->b_blkno = blkno;
		return(b);
	}

	/*
	 * Oops.  This really shouldn't be possible in a single-threaded
	 * server, unless we're leaking block references.
	 */
	ASSERT(0, "bnew: all blocks busy");
	return(0);
}

/*
 * bfind()
 *	Try and find a block in the cache
 *
 * If it's found, it is returned with the b_refs incremented.  On failure,
 * a NULL pointer is returned.
 */
static struct block *
bfind(int blkno)
{
	struct block *b;
	struct llist *l, *lfirst;

	lfirst = &hash[blkno & HASHMASK];
	for (l = lfirst->l_forw; l != lfirst; l = l->l_forw) {
		b = l->l_data;
		if (blkno == b->b_blkno) {
			b->b_refs += 1;
			return(b);
		}
	}
	return(0);
}

/*
 * bdirty()
 *	Flag buffer as dirty--it must be written before reused
 */
void
bdirty(void *bp)
{
	struct block *b = bp;

	ASSERT_DEBUG(b, "bdirty: NULL");
	ASSERT_DEBUG(b->b_refs > 0, "bdirty: no ref");
	b->b_flags |= B_DIRTY;
}

/*
 * binval()
 *	Throw out a buf header, usually to recover from I/O errors
 */
static void
bjunk(struct block *b)
{
	ll_delete(b->b_hash);
	ll_delete(b->b_all);
	free(b->b_data);
	free(b);
	cached -= 1;
}

/*
 * bget()
 *	Find block in cache or read from disk; return pointer
 *
 * On success, an opaque pointer is returned.  On error,
 * NULL is returned.
 * The new block has a reference added to it.
 */
void *
bget(int blkno)
{
	struct block *b;
	int x;

	b = bfind(blkno);
	if (!b) {
		b = bnew(blkno);
		if (lseek(blkdev, BOFF(blkno), 0) == -1) {
			bjunk(b);
			return(0);
		}
		x = read(blkdev, b->b_data, BLOCKSIZE);
		if (x != BLOCKSIZE) {
			bjunk(b);
			return(0);
		}
	}
	return(b);
}

/*
 * bdata()
 *	Convert opaque pointer into corresponding data
 */
void *
bdata(void *bp)
{
	struct block *b = bp;

	ASSERT_DEBUG(b->b_refs > 0, "bdata: no ref");
	return(b->b_data);
}

/*
 * bfree()
 *	Indicate that the current reference is complete
 */
void
bfree(void *bp)
{
	struct block *b = bp;

	ASSERT_DEBUG(b->b_refs > 0, "bfree: free and no ref");
	b->b_refs -= 1;
}

/*
 * binit()
 *	Initialize our buffer hash chains
 */
void
binit(void)
{
	int x;

	for (x = 0; x < HASHSIZE; ++x)
		ll_init(&hash[x]);
	ll_init(&allblocks);
}

/*
 * bsync()
 *	Flush out all dirty buffers
 */
void
bsync(void)
{
	struct llist *l;
	struct block *b;

	for (l = allblocks.l_forw; l != &allblocks; l = l->l_forw) {
		b = l->l_data;
		if (b->b_flags & B_DIRTY) {
			lseek(blkdev, BOFF(b->b_blkno), 0);
			write(blkdev, b->b_data, BLOCKSIZE);
			b->b_flags &= ~B_DIRTY;
		}
	}
}
@


1.6
log
@Assert on NULL-ness for nice debug printf
@
text
@a10 1
char *errstr;			/* String for last error */
d194 1
a194 1
 * NULL is returned, and the error is recorded in "errstr".
a207 1
			errstr = strerror();
a212 1
			errstr = strerror();
@


1.5
log
@Fix -Wall warnings
@
text
@d171 1
@


1.4
log
@Fix bugs in block handling
@
text
@d9 1
a9 2

extern int blkdev;
@


1.3
log
@Mistake in buffer reuse.  Forgot to change blkno of buf when
it got stolen.
@
text
@d79 1
d113 1
a113 1
		 * Update our "all block" header to point one past us
d115 3
a117 1
		ll_movehead(&allblocks, l->l_forw);
@


1.2
log
@Source reorg
@
text
@d124 1
@


1.1
log
@Initial revision
@
text
@d5 2
a6 2
#include <dos/dos.h>
#include <lib/llist.h>
@
