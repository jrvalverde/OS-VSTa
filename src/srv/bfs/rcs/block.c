head	1.6;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.4
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.6
date	94.03.08.20.04.21;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.02.02.19.57.55;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.12.22.00.22.13;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.47.08;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.10.20.30.37;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.38.39;	author vandys;	state Exp;
branches;
next	;


desc
@Block cache
@


1.6
log
@Rev boot filesystem per work from Dave Hudson
@
text
@/*
 * Filename:	block.c
 * Developed:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Originated:	Andy Valencia
 * Last Update: 13th February 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description:	Routines for caching blocks out of our filesystem
 */


#include <fcntl.h>
#include <llist.h>
#include <std.h>
#include <stdio.h>
#include <sys/assert.h>
#include "bfs.h"


extern int blkdev;

char *errstr;			/* String for last error */
static int cached = 0;		/* # blocks currently cached */


/*
 * Structure describing a block in the cache
 */
struct block {
	int b_blkno;			/* Block we describe */
	int b_flags;			/* Flags (see below) */
	void *b_data;			/* Pointer to data */
	struct llist *b_all;		/* Circular list of all blocks */
	struct llist *b_hash;		/* Linked list on hash collision */
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
				return b;
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
			lseek(blkdev, b->b_blkno*(long)BLOCKSIZE, 0);
			write(blkdev, b->b_data, BLOCKSIZE);
			b->b_flags &= ~B_DIRTY;
		}

		/*
		 * Update our "all block" header to point one past us
		 */
		ll_movehead(&allblocks, l->l_forw);

		/*
		 * Update the hash chain it resides on, and return it.
		 * Our caller is responsible for putting the right
		 * data into the buffer.
		 */
		ll_delete(b->b_hash);
		b->b_hash = ll_insert(&hash[blkno & HASHMASK], b);
		b->b_refs = 1;
		b->b_blkno = blkno;
		return b;
	}

	/*
	 * Oops.  This really shouldn't be possible in a single-threaded
	 * server, unless we're leaking block references.
	 */
	ASSERT(0, "bnew: all blocks busy");
	return NULL;
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
			return b;
		}
	}
	return NULL;
}


/*
 * bdirty()
 *	Flag buffer as dirty--it must be written before reused
 */
void
bdirty(void *bp)
{
	struct block *b = bp;

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
 * NULL is returned, and the error is recorded in "errstr".
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
		if (lseek(blkdev, blkno*(long)BLOCKSIZE, 0) == -1) {
			bjunk(b);
			errstr = strerror();
			return NULL;
		}
		x = read(blkdev, b->b_data, BLOCKSIZE);
		if (x != BLOCKSIZE) {
			bjunk(b);
			errstr = strerror();
			return NULL;
		}
	}
	return b;
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
	return b->b_data;
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
			lseek(blkdev, b->b_blkno*(long)BLOCKSIZE, 0);
			write(blkdev, b->b_data, BLOCKSIZE);
			b->b_flags &= ~B_DIRTY;
		}
	}
}
@


1.5
log
@Fix bugs in block handling
@
text
@d2 7
a8 2
 * block.c
 *	Routines for caching blocks out of our filesystem
d10 3
a12 1
#include "bfs.h"
d14 2
d17 1
a18 2
extern void *malloc();
extern char *strerror();
d25 1
d33 2
a34 2
	struct llist *b_all,		/* Circular list of all blocks */
		*b_hash;		/* Linked list on hash collision */
d38 1
d44 1
d51 2
d56 1
d89 1
a89 2
				cached += 1;
				return(b);
d124 1
a124 3
		ll_delete(b->b_all);
		b->b_all = ll_insert(&allblocks, b);
		ASSERT_DEBUG(b->b_all, "bnew: lost the llist");
d135 1
a135 1
		return(b);
d143 1
a143 1
	return(0);
d146 1
d165 1
a165 1
			return(b);
d168 1
a168 1
	return(0);
d171 1
d185 1
d200 1
d221 1
a221 1
			return(0);
d227 1
a227 1
			return(0);
d230 1
a230 1
	return(b);
d233 1
d244 1
a244 1
	return(b->b_data);
d247 1
d261 1
d275 1
@


1.4
log
@Mistake in buffer resuse
@
text
@d75 1
d111 3
a113 1
		ll_movehead(&allblocks, l->l_forw);
@


1.3
log
@Source reorg
@
text
@d120 1
@


1.2
log
@Use file descriptor layer; don't try to use ports directly
@
text
@d5 2
a6 2
#include <bfs/bfs.h>
#include <lib/llist.h>
@


1.1
log
@Initial revision
@
text
@d12 1
a12 1
extern port_t blkdev;
d102 2
a103 2
			__seek(blkdev, b->b_blkno*BLOCKSIZE);
			__write(blkdev, b->b_data, BLOCKSIZE);
d199 1
a199 1
		if (__seek(blkdev, blkno*BLOCKSIZE)) {
d204 1
a204 1
		x = __read(blkdev, b->b_data, BLOCKSIZE);
d267 2
a268 2
			__seek(blkdev, b->b_blkno*BLOCKSIZE);
			__write(blkdev, b->b_data, BLOCKSIZE);
@
