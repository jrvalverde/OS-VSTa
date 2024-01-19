head	1.10;
access;
symbols
	V1_3_1:1.10
	V1_3:1.10
	V1_2:1.9
	V1_1:1.9;
locks; strict;
comment	@ * @;


1.10
date	94.02.07.19.40.28;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.10.03.00.26.56;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.09.27.23.09.29;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.09.12.23.36.22;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.31.03.05.46;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.19.12.00;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.27.15.39.58;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.27.13.41.52;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.08.59.33;	author vandys;	state Exp;
branches;
next	;


desc
@Open-file node handling
@


1.10
log
@Record value as data might move on buffer resize
@
text
@/*
 * node.c
 *	Handling of open file nodes
 */
#include "vstafs.h"
#include "buf.h"
#include <sys/assert.h>
#include <hash.h>
#include <std.h>

static struct hash *node_hash;

/*
 * init_node()
 *	Initialize node system
 */
void
init_node(void)
{
	node_hash = hash_alloc(NCACHE);
	ASSERT(node_hash, "init_node: out of memory");
}

/*
 * ref_node()
 *	Add a reference to a node
 */
void
ref_node(struct openfile *o)
{
	o->o_refs += 1;
	ASSERT_DEBUG(o->o_refs > 0, "ref_node: overflow");
}

/*
 * deref_node()
 *	Remove a reference, free on last reference
 */
void
deref_node(struct openfile *o)
{
	ASSERT_DEBUG(o->o_refs > 0, "deref_node: zero");
	o->o_refs -= 1;
	if (o->o_refs == 0) {
		ASSERT(hash_delete(node_hash, o->o_file) == 0,
			"deref_node: hash mismatch");
		free(o);
	}
}

/*
 * alloc_node()
 *	Allocate a new node
 *
 * Somewhat complicated because the file's storage structure is stored
 * at the front of the file.  We read in the first sector, then extend
 * the buffered block out to the indicated size.
 */
static struct openfile *
alloc_node(daddr_t d)
{
	struct buf *b;
	struct fs_file *fs;
	ulong len, fslen;
	struct openfile *o;

	/*
	 * Get buf, address first sector as an fs_file
	 */
	b = find_buf(d, 1);
	fs = index_buf(b, 0, 1);
	ASSERT(fs->fs_nblk > 0, "alloc_node: zero");
	ASSERT(fs->fs_blks[0].a_start == d, "alloc_node: mismatch");

	/*
	 * Extend the buf if necessary
	 */
	len = fs->fs_blks[0].a_len;
	fslen = fs->fs_len;
	if (len > 1) {
		if (len > EXTSIZ) {
			len = EXTSIZ;
		}
		if (resize_buf(d, (uint)len, 1)) {
			return(0);
		}
	}

	/*
	 * Create a new openfile and return it
	 */
	o = malloc(sizeof(struct openfile));
	if (o == 0) {
		return(0);
	}
	o->o_file = d;
	o->o_len = len;
	o->o_hiwrite = fslen;
	o->o_refs = 1;
	return(o);
}

/*
 * get_node()
 *	Return node, either from hash or created from scratch
 */
struct openfile *
get_node(daddr_t d)
{
	struct openfile *o;

	/*
	 * From hash?
	 */
	o = hash_lookup(node_hash, d);
	if (o) {
		/*
		 * Yes--add a reference, and return
		 */
		ref_node(o);
		return(o);
	}

	/*
	 * Get a new one, and return it
	 */
	o = alloc_node(d);
	if (hash_insert(node_hash, d, o)) {
		deref_node(o);
		o = 0;
	}
	return(o);
}
@


1.9
log
@Source reorg
@
text
@d64 1
a64 1
	ulong len;
d79 1
d98 1
a98 1
	o->o_hiwrite = fs->fs_len;
@


1.8
log
@Set o_hiwrite from byte length, not sector length
@
text
@d5 2
a6 2
#include <vstafs/vstafs.h>
#include <vstafs/buf.h>
d8 1
a8 1
#include <lib/hash.h>
@


1.7
log
@Add o_hiwrite
@
text
@d96 2
a97 1
	o->o_hiwrite = o->o_len = len;
@


1.6
log
@Rename to resize_buf
@
text
@d96 1
a96 1
	o->o_len = len;
@


1.5
log
@Add missing hash deletion on last reference.  Use central
ref counting.
@
text
@d83 1
a83 1
		if (extend_buf(d, (uint)len, 1)) {
@


1.4
log
@Convert accesses to fs_file to use the routine getfs()
@
text
@d45 2
d118 1
a118 2
		o->o_refs += 1;
		ASSERT(o->o_refs > 1, "get_node: bad o_refs");
@


1.3
log
@Add init_node(), add node hash lookup
@
text
@d20 1
a20 1
	node_hash = alloc_hash(NCACHE);
d103 1
@


1.2
log
@Convert buffer resize to use only block addresses.  Do rest of
read/write support.
@
text
@d8 1
d11 13
d57 1
a57 1
struct openfile *
d96 32
@


1.1
log
@Initial revision
@
text
@d64 4
a67 1
		if (extend_buf(b, len, 1)) {
@
