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
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.13.17.13.13;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.30.00;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.25.17.05.33;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of the nodes which comprise our "filesystem"
@


1.4
log
@Source reorg
@
text
@/*
 * node.c
 *	Handling of nodes in filesystem
 */
#include "env.h"
#include <std.h>
#include <sys/assert.h>
#include <sys/fs.h>

/*
 * alloc_node()
 *	Allocate a new node, attach under current directory
 *
 * Although it's attached, it is not visible.  The caller must
 * add it to the parent's linked list after assigning this node
 * a name.
 */
struct node *
alloc_node(struct file *f)
{
	struct node *n;
	struct prot *p;

	if ((n = malloc(sizeof(struct node))) == 0) {
		return(0);
	}

	/*
	 * Default protection is first user ID with all access
	 * limited to that ID.
	 */
	p = &n->n_prot;
	bzero(p, sizeof(*p));
	p->prot_len = PERM_LEN(&f->f_perms[0]);
	bcopy(f->f_perms[0].perm_id, p->prot_id, PERMLEN);
	p->prot_bits[p->prot_len-1] =
		ACC_READ|ACC_WRITE|ACC_CHMOD;
	n->n_owner = f->f_perms[0].perm_uid;

	/*
	 * Initialize rest of fields
	 */
	n->n_name[0] = '\0';
	n->n_flags = 0;
	n->n_list = 0;
	n->n_val = 0;
	n->n_up = f->f_node; ref_node(f->f_node);
	n->n_refs = 0;
	ll_init(&n->n_elems);
	return(n);
}

/*
 * clone_node()
 *	Set up a copy-on-write image of the given node
 */
struct node *
clone_node(struct node *nold)
{
	struct node *n, *n2, *n3;
	struct llist *l;

	/*
	 * Get storage, duplicate most fields
	 */
	if ((n = malloc(sizeof(struct node))) == 0) {
		return(0);
	}
	bcopy(nold, n, sizeof(struct node));
	n->n_refs = 1;
	ll_init(&n->n_elems);
	n->n_list = 0;

	/*
	 * Attach to parent
	 */
	ref_node(n->n_up);

	/*
	 * Walk contents, attach to values
	 */
	for (l = LL_NEXT(&nold->n_elems);
			l != &nold->n_elems; l = LL_NEXT(l)) {
		/*
		 * Get new node
		 */
		n2 = l->l_data;
		n3 = malloc(sizeof(struct node));
		if (n3 == 0) {
			continue;
		}

		/*
		 * Copy most fields, attach to value
		 */
		*n3 = *n2;
		ref_val(n3->n_val);

		/*
		 * Add to our new list
		 */
		if (!(n3->n_list = ll_insert(&n->n_elems, n3))) {
			deref_val(n3->n_val);
			free(n3);
		}
	}
	return(n);
}

/*
 * ref_node()
 *	Add a reference to the given node
 */
void
ref_node(struct node *n)
{
	if (n == 0) {
		return;
	}
	n->n_refs += 1;
}

/*
 * deref_node()
 *	Remove reference, free storage on last reference
 */
void
deref_node(struct node *n)
{
	extern struct node rootnode;

	/*
	 * Null--no node, ignore
	 */
	if (n == 0) {
		return;
	}

	/*
	 * More refs--easy
	 */
	n->n_refs -= 1;
	if (n->n_refs > 0) {
		return;
	}
	ASSERT_DEBUG(n != &rootnode, "deref_node: root !ref");
	ASSERT(n->n_elems.l_forw == &n->n_elems, "deref_node: elems !ref");

	/*
	 * Free ref to value, parent
	 */
	deref_val(n->n_val);
	deref_node(n->n_up);

	/*
	 * Free storage
	 */
	free(n);
}

/*
 * remove_node()
 *	Discard contents, then free storage
 */
void
remove_node(struct node *n)
{
	struct llist *l;

	/*
	 * Null--no node, ignore
	 */
	if (n == 0) {
		return;
	}

	/*
	 * Internal (directory) nodes need to have their
	 * elements freed.
	 */
	if (DIR(n)) {
		while (!LL_EMPTY(&n->n_elems)) {
			l = LL_NEXT(&n->n_elems);
			remove_node(l->l_data);
		}
	}

	/*
	 * Remove from parent's list if present, free string
	 * value, then free our storage.
	 */
	if (n->n_list) {
		ll_delete(n->n_list);
	}
	deref_val(n->n_val);
	free(n);
}
@


1.3
log
@Remove redundant ll_delete during tear-down
@
text
@d5 1
a5 1
#include <env/env.h>
a6 1
#include <lib/llist.h>
@


1.2
log
@Add UID tag, get from client
@
text
@a169 1
	struct node *n2;
d185 1
a185 3
			n2 = l->l_data;
			remove_node(n2);
			ll_delete(l);
@


1.1
log
@Initial revision
@
text
@d35 1
a35 1
	p->prot_len = f->f_perms[0].perm_len;
d39 1
@
