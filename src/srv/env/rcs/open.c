head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.3
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.17.03.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.05.16.02.28;	author vandys;	state Exp;
branches;
next	;


desc
@Open/close/remove/create support
@


1.3
log
@Source reorg
@
text
@/*
 * open.c
 *	Routines for moving downwards in the hierarchy
 */
#include <sys/types.h>
#include "env.h"
#include <sys/fs.h>
#include <sys/assert.h>
#include <std.h>

/*
 * lookup()
 *	Look through list for a name
 */
static struct node *
lookup(struct file *f, char *name, int searchup)
{
	struct node *n, *n2;
	struct llist *l;

	n = f->f_node;
	ASSERT_DEBUG(DIR(n), "env lookup: not a dir");
	for (; n; n = n->n_up) {
		for (l = LL_NEXT(&n->n_elems);
				l != &n->n_elems; l = LL_NEXT(l)) {
			n2 = l->l_data;
			if (!strcmp(n2->n_name, name)) {
				return(n2);
			}
		}

		/*
		 * If not allowed to search upwards in tree, fail
		 * now.
		 */
		if (!searchup) {
			return(0);
		}
	}
	return(0);
}

/*
 * env_open()
 *	Look up an entry downward
 */
void
env_open(struct msg *m, struct file *f)
{
	struct node *n, *nold = f->f_node;
	int creating = (m->m_arg & ACC_CREATE), home;
	char *nm;

	/*
	 * Make sure it's a "directory", cap length.
	 */
	if (!DIR(nold) || (strlen(m->m_buf) >= NAMESZ)) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * "#" refers to a "magic" copy-on-write invisible node
	 */
	nm = m->m_buf;
	if (!strcmp(nm, "#")) {
		home = 1;
	} else {
		home = 0;
	}

	/*
	 * See if we can find an existing entry
	 */
	if (!home) {
		n = lookup(f, nm, !creating);
	} else {
		n = f->f_home;
	}

	/*
	 * If found, verify access and type of use
	 */
	if (n) {
		if (access(f, m->m_arg, &n->n_prot)) {
			msg_err(m->m_sender, EPERM);
			return;
		}

		/*
		 * If creating on an existing file, truncate
		 */
		if ((creating) && !DIR(n)) {
			deref_val(n->n_val);
			n->n_val = alloc_val("");
		}

		/*
		 * Move current reference to new node
		 */
		deref_node(nold);
		f->f_node = n;
		ref_node(n);
		f->f_pos = 0L;
		m->m_buflen = m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * If not intending to create, error
	 */
	if (!creating) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * If we don't have write access to the current node,
	 * error.
	 */
	if (!(f->f_mode & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Get the new node
	 */
	n = alloc_node(f);
	if (m->m_arg & ACC_DIR) {
		n->n_flags |= N_INTERNAL;
	} else {
		n->n_val = alloc_val("");
	}

	/*
	 * Try inserting it under the current node
	 */
	if (!home) {
		strcpy(n->n_name, nm);
		ref_node(n);
		if (!(n->n_list = ll_insert(&nold->n_elems, n))) {
			deref_node(n);
			deref_node(nold);
			msg_err(m->m_sender, ENOMEM);
			return;
		}
		/* Reference to nold done in alloc_node (n->n_up) */
	} else {
		struct file *f2;

		/*
		 * Switch home for all in this group
		 */
		f2 = f;
		do {
			deref_node(f2->f_home);
			f2->f_home = n;
			ref_node(n);
			f2 = f2->f_forw;
		} while (f2 != f);
	}

	/*
	 * Move "f" down to new node
	 */
	deref_node(nold);
	f->f_node = n;
	ref_node(n);

	m->m_buflen = m->m_nseg = m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * env_remove()
 *	Delete an entry from the current directory
 */
void
env_remove(struct msg *m, struct file *f)
{
	struct node *n = f->f_node, *n2;

	/*
	 * Make sure we're in a "directory"
	 */
	if (!DIR(n)) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * See if we have write access
	 */
	if (!(f->f_mode & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * See if we can find the entry
	 */
	n2 = lookup(f, m->m_buf, 0);

	/*
	 * If not found, forget it
	 */
	if (!n2) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * If the node is busy, forget it
	 */
	if (n2->n_refs > 1) {
		msg_err(m->m_sender, EBUSY);
		return;
	}
	ASSERT((n2->n_refs == 1) && (n->n_refs > 2),
		"env_remove: short ref");

	/*
	 * Let the node disappear
	 */
	deref_node(n2);

	m->m_buflen = m->m_nseg = m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}
@


1.2
log
@Rework inheritance of environment
@
text
@d6 1
a6 1
#include <env/env.h>
a8 1
#include <lib/llist.h>
@


1.1
log
@Initial revision
@
text
@d23 1
a23 1
	ASSERT_DEBUG(n->n_internal, "env lookup: not a dir");
d25 2
a26 2
		for (l = n->n_elems.l_forw;
				l != &n->n_elems; l = l->l_forw) {
d51 3
a53 3
	struct node *n;
	struct prot *p;
	int creating = (m->m_arg & ACC_CREATE);
d56 1
a56 1
	 * Make sure it's a "directory"
d58 1
a58 1
	if (!f->f_node->n_internal) {
d64 1
a64 1
	 * See if you can find an existing entry
d66 15
a80 1
	n = lookup(f, m->m_buf, !creating);
d94 1
a94 1
		if (creating) {
d102 1
a102 1
		f->f_node->n_refs -= 1;
d104 1
a104 1
		n->n_refs += 1;
d129 1
a129 1
	 * malloc the new node
d131 5
a135 3
	if ((n = malloc(sizeof(struct node))) == 0) {
		msg_err(m->m_sender, ENOMEM);
		return;
d141 23
a163 4
	if (!(n->n_list = ll_insert(&f->f_node->n_elems, n))) {
		free(n);
		msg_err(m->m_sender, ENOMEM);
		return;
d167 1
a167 19
	 * Fill in its fields.  Default label is the first user
	 * label, with all the abilities requiring a full match.
	 */
	p = &n->n_prot;
	bzero(p, sizeof(*p));
	p->prot_len = f->f_perms[0].perm_len;
	bcopy(f->f_perms[0].perm_id, p->prot_id, PERMLEN);
	p->prot_bits[p->prot_len-1] =
		ACC_READ|ACC_WRITE|ACC_CHMOD;
	strcpy(n->n_name, m->m_buf);
	n->n_internal = (m->m_arg & ACC_DIR) ? 1 : 0;
	n->n_val = alloc_val("");
	n->n_up = f->f_node;
	ll_init(&n->n_elems);
	n->n_refs = 2;	/* One from f->f_node, another for this open */

	/*
	 * Move "f" down to new node.  Note that we leave a reference
	 * on the parent--it is now the reference of the child node.
d169 1
d171 1
a184 1
	struct prot *p;
d189 1
a189 1
	if (!n->n_internal) {
d226 1
a226 7
	 * Trim reference counts, remove node from list, free memory.
	 */
	n->n_refs -= 1;
	ll_delete(n2->n_list);

	/*
	 * Release reference to string memory, free this node
d228 1
a228 2
	deref_val(n2->n_val);
	free(n2);
@
