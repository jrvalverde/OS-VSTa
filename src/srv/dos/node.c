/*
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
