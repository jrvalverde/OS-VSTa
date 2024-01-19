head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.4
date	93.11.16.02.50.17;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.02.21.49.21;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.21.21.08;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.56.40;	author vandys;	state Exp;
branches;
next	;


desc
@A hashed-lookup object
@


1.4
log
@Source reorg
@
text
@/*
 * hash.c
 *	A hashed lookup mechanism
 */
#include <std.h>
#include <hash.h>

/*
 * hashval()
 *	Convert key into hash value
 */
static uint
hashidx(ulong key, uint size)
{
	return((key ^ (key >> 2)) % size);
}

/*
 * hash_alloc()
 *	Allocate a hash data structure of the given hash size
 */
struct hash *
hash_alloc(int hashsize)
{
	struct hash *h;

	h = malloc(sizeof(struct hash) + hashsize*sizeof(struct hash *));
	if (h) {
		h->h_hashsize = hashsize;
		bzero(&h->h_hash, hashsize*sizeof(struct hash *));
	}
	return(h);
}

/*
 * hash_insert()
 *	Insert a new key/value pair into the hash
 *
 * Returns 1 on error, 0 on success.
 */
hash_insert(struct hash *h, long key, void *val)
{
	struct hash_node *hn;
	uint idx;

	if (!h) {
		return(1);
	}
	idx = hashidx(key, h->h_hashsize);
	hn = malloc(sizeof(struct hash_node));
	if (!hn) {
		return(1);
	}
	hn->h_key = key;
	hn->h_data = val;
	hn->h_next = h->h_hash[idx];
	h->h_hash[idx] = hn;
	return(0);
}

/*
 * hash_delete()
 *	Remove node from hash
 *
 * Returns 1 if key not found, 0 if removed successfully.
 */
hash_delete(struct hash *h, long key)
{
	struct hash_node **hnp, *hn;
	uint idx;

	if (!h) {
		return(1);
	}

	/*
	 * Walk hash chain list.  Walk both the pointer, as
	 * well as a pointer to the previous pointer.  When
	 * we find the node, patch out the current node and
	 * free it.
	 */
	idx = hashidx(key, h->h_hashsize);
	hnp = &h->h_hash[idx];
	hn = *hnp;
	while (hn) {
		if (hn->h_key == key) {
			*hnp = hn->h_next;
			free(hn);
			return(0);
		}
		hnp = &hn->h_next;
		hn = *hnp;
	}
	return(1);
}

/*
 * hash_dealloc()
 *	Free up the entire hash structure
 */
void
hash_dealloc(struct hash *h)
{
	uint x;
	struct hash_node *hn, *hnn;

	for (x = 0; x < h->h_hashsize; ++x) {
		for (hn = h->h_hash[x]; hn; hn = hnn) {
			hnn = hn->h_next;
			free(hn);
		}
	}
	free(h);
}

/*
 * hash_lookup()
 *	Look up a node based on its key
 */
void *
hash_lookup(struct hash *h, long key)
{
	struct hash_node *hn;
	uint idx;

	if (!h) {
		return(0);
	}
	idx = hashidx(key, h->h_hashsize);
	for (hn = h->h_hash[idx]; hn; hn = hn->h_next) {
		if (hn->h_key == key) {
			return(hn->h_data);
		}
	}
	return(0);
}

/*
 * hash_size()
 *	Tell how many elements are stored in the hash
 */
uint
hash_size(struct hash *h)
{
	uint x, cnt = 0;
	struct hash_node *hn;

	for (x = 0; x < h->h_hashsize; ++x) {
		for (hn = h->h_hash[x]; hn; hn = hn->h_next) {
			cnt += 1;
		}
	}
	return(cnt);
}

/*
 * hash_foreach()
 *	Enumerate each entry in the hash, invoking a function
 */
void
hash_foreach(struct hash *h, intfun f, void *arg)
{
	uint x;
	struct hash_node *hn;

	for (x = 0; x < h->h_hashsize; ++x) {
		for (hn = h->h_hash[x]; hn; hn = hn->h_next) {
			if ((*f)(hn->h_key, hn->h_data, arg)) {
				return;
			}
		}
	}
}
@


1.3
log
@Create common hash function.  XOR in a shifted version of the index;
this improvies hashing when the index is actually a pointer which
has been rounded to a 4-byte boundary.
@
text
@d6 1
a6 1
#include <lib/hash.h>
@


1.2
log
@Add new functions
@
text
@d9 10
d44 1
a44 1
	int idx;
d49 1
a49 1
	idx = key % h->h_hashsize;
d51 1
a51 1
	if (!hn)
d53 1
d70 1
a70 1
	int idx;
d82 1
a82 1
	idx = key % h->h_hashsize;
d104 1
a104 1
	int x;
d124 1
d129 3
a131 2
	for (hn = h->h_hash[key % h->h_hashsize]; hn; hn = hn->h_next) {
		if (hn->h_key == key)
d133 1
@


1.1
log
@Initial revision
@
text
@d5 2
a7 21
extern void *malloc();

/*
 * This is the root of the hashed object.  You shouldn't monkey
 * with it directly.
 */
struct hash {
	int h_hashsize;		/* Width of h_hash array */
	struct hash_node	/* Chains under each hash value */
		*h_hash[1];
};

/*
 * Hash collision chains.  An internal data structure.
 */
struct hash_node {
	struct hash_node *h_next;	/* Next on hash chain */
	long h_key;			/* Key for this node */
	void *h_data;			/*  ...corresponding value */
};

d122 37
@
