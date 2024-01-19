head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.3
date	93.03.16.19.11.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.21.20.45;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.56.52;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for hash library routines
@


1.3
log
@Map hash entries into POSIX-protected name space to avoid
collisions with apps.
@
text
@#ifndef _HASH_H
#define _HASH_H
/*
 * hash.h
 *	Data structures for a hashed lookup object
 */
#include <sys/types.h>

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

/*
 * Smoke and mirrors to avoid name space pollution
 */
#define hash_alloc __hash_alloc
#define hash_insert __hash_insert
#define hash_delete __hash_delete
#define hash_lookup __hash_lookup
#define hash_dealloc __hash_dealloc
#define hash_foreach __hash_foreach
#define hash_size __hash_size

/*
 * Hash routines
 */
struct hash *hash_alloc(int);
int hash_insert(struct hash *, long, void *);
int hash_delete(struct hash *, long);
void *hash_lookup(struct hash *, long);
void hash_dealloc(struct hash *);
void hash_foreach(struct hash *, intfun, void *);
uint hash_size(struct hash *);

#endif /* _HASH_H */
@


1.2
log
@Add prototypes for new functions, include types.h as we use
system types now.
@
text
@d29 11
@


1.1
log
@Initial revision
@
text
@d7 1
d36 2
@
