head	1.12;
access;
symbols
	V1_3_1:1.9
	V1_3:1.8
	V1_2:1.7
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.12
date	95.02.04.05.53.47;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.12.21.16.47.59;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.09.23.20.36.37;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.20.21.05.43;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.03.23.21.58.06;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.48.09;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.07.21.15.09;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.16.22.46.25;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.22.17.58.54;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.22.14.50.26;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.21.43.49;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of in-core file information
@


1.12
log
@Add firewall that hash is empty under dir when all files
under it are closed
@
text
@/*
 * node.c
 *	Handling for dir/file nodes
 */
#include "dos.h"
#include <sys/assert.h>
#include <hash.h>
#include <std.h>

/*
 * ref_node()
 *	Add a reference to a node
 */
void
ref_node(struct node *n)
{
	n->n_refs += 1;
	ASSERT(n->n_refs != 0, "ref_node: overflow");
}

/*
 * deref_node()
 *	Remove a reference from a node
 *
 * Flush on last close of a dirty file
 */
void
deref_node(struct node *n)
{
	struct clust *c;

	ASSERT(n->n_refs > 0, "deref_node: no refs");

	/*
	 * Remove ref
	 */
	if ((n->n_refs -= 1) > 0) {
		/*
		 * If still open, checkpoint file state into
		 * directory entry if any changes have been made.
		 */
		if (n->n_flags & N_DIRTY) {
			dir_setlen(n);
			sync();
			n->n_flags &= ~N_DIRTY;
		}
		return;
	}

	/*
	 * Never touch root
	 */
	if (n == rootdir) {
		return;
	}

	/*
	 * If he's already deleted, much of this should be skipped
	 */
	c = n->n_clust;
	if (!(n->n_flags & N_DEL)) {
		/*
		 * Flush out dirty blocks on each last close
		 */
		if (n->n_flags & N_DIRTY) {
			dir_setlen(n);
			sync();
		}

		/*
		 * If file, remove ref from dir we're within
		 */
		if ((n->n_type == T_FILE) || (n->n_type == T_SYM)) {
			hash_delete(n->n_dir->n_files, n->n_slot);
		} else {
			extern struct hash *dirhash;

			ASSERT_DEBUG(n->n_type == T_DIR,
				"deref_node: bad type");
			ASSERT(c->c_nclust > 0, "deref_node: short dir");
			hash_delete(dirhash, c->c_clust[0]);
		}
	} else {
		ASSERT_DEBUG(c->c_nclust == 0, "node_deref: del w. clusters");

		/*
		 * At least the containing directory will be dirty
		 */
		sync();
	}

	/*
	 * Release reference to containing node
	 */
	deref_node(n->n_dir);

	/*
	 * Free FAT cache
	 */
	free_clust(c);

	/*
	 * Free file hash if a dir
	 */
	if (n->n_type == T_DIR) {
		ASSERT_DEBUG(hash_size(n->n_files) == 0,
			"deref_node: dir && !empty");
		hash_dealloc(n->n_files);
	}

	/*
	 * Free our memory
	 */
	free(n);
}
@


1.11
log
@Free file hash when freeing dir node
@
text
@d106 2
@


1.10
log
@Add symlink support
@
text
@d103 7
@


1.9
log
@Fix flushing dirty nodes out on close()
@
text
@d73 1
a73 1
		if (n->n_type == T_FILE) {
@


1.8
log
@Fix -Wall warnings
@
text
@d35 1
a35 1
	 * Remove ref, do nothing if still open
d38 9
@


1.7
log
@Source reorg
@
text
@d8 1
@


1.6
log
@Inhibit some cleanup when it's a node which has been
deleted.
@
text
@d5 1
a5 1
#include <dos/dos.h>
d7 1
a7 1
#include <lib/hash.h>
@


1.5
log
@Include hash.h to get prototypes
@
text
@d48 1
a48 1
	 * Flush out dirty blocks on each last close
d50 29
a78 2
	if (n->n_flags & N_DIRTY) {
		dir_setlen(n);
d83 1
a83 1
	 * If file, remove ref from dir we're within
d85 1
a85 11
	c = n->n_clust;
	if (n->n_type == T_FILE) {
		hash_delete(n->n_dir->n_files, n->n_slot);
		deref_node(n->n_dir);
	} else {
		extern struct hash *dirhash;

		ASSERT_DEBUG(n->n_type == T_DIR, "deref_node: bad type");
		ASSERT(c->c_nclust > 0, "deref_node: short dir");
		hash_delete(dirhash, c->c_clust[0]);
	}
@


1.4
log
@Add a little sanity check on ref_node(); floating reference
counts which wrap can cause really subtle bugs.
@
text
@d7 1
@


1.3
log
@Clean up on deref of dirnode, too
@
text
@d16 1
@


1.2
log
@Convet to sync(), which flushes out more stuff which needs
to be flushed.
@
text
@d27 2
d39 7
a53 5
	 * Free FAT cache
	 */
	free_clust(n->n_clust);

	/*
d56 1
d60 6
d67 5
@


1.1
log
@Initial revision
@
text
@d37 1
a37 1
	 * Flush out dirty blocks on last close
d40 2
a41 1
		bsync();	/* XXX Should only sync this file & dir */
@
