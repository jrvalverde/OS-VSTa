head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.5
	V1_1:1.5
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.5
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.10.03.02.49.10;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.23.30.00;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.17.04.11;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.05.16.02.28;	author vandys;	state Exp;
branches;
next	;


desc
@STat handling
@


1.5
log
@Source reorg
@
text
@/*
 * stat.c
 *	Do the stat function
 *
 * We also lump the chmod/chown stuff here as well
 */
#include "env.h"
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>

extern char *perm_print();

/*
 * env_stat()
 *	Do stat
 */
void
env_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];
	struct node *n = f->f_node;
	int len;
	struct llist *l;

	/*
	 * Verify access
	 */
	if (!(f->f_mode & ACC_READ)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Calculate length
	 */
	if (DIR(n)) {
		len = 0;
		l = LL_NEXT(&n->n_elems);
		while (l != &n->n_elems) {
			len += 1;
			l = LL_NEXT(l);
		}
	} else {
		len = strlen(n->n_val->s_val);
	}
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%u\n",
		len, DIR(n) ? 'd' : 'f', n->n_owner, n);
	strcat(buf, perm_print(&n->n_prot));
	m->m_buf = buf;
	m->m_arg = m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * env_fork()
 *	Spawn off a copy-on-write node
 */
static void
env_fork(struct file *f)
{
	struct node *oldhome = f->f_home;

	/*
	 * Leave current group, start a new one
	 */
	f->f_back->f_forw = f->f_forw;
	f->f_forw->f_back = f->f_back;
	f->f_forw = f->f_back = f;

	/*
	 * Set up copy-on-write of home node if present
	 */
	if (oldhome) {
		f->f_home = clone_node(oldhome);
		deref_node(oldhome);
		/* clone_node has put first reference in place */
	}

	/*
	 * Set current node as needed.  If the current node of our
	 * old node was his "home" node, then we switch to our *own*
	 * home node instead of pointing at his.
	 */
	if (f->f_node == oldhome) {
		deref_node(f->f_node);
		if (f->f_home) {
			f->f_node = f->f_home;
		} else {
			extern struct node rootnode;

			/*
			 * Couldn't copy, just switch to root
			 */
			f->f_node = &rootnode;
		}
		ref_node(f->f_node);
	}
}

/*
 * env_wstat()
 *	Allow writing of supported stat messages
 */
void
env_wstat(struct msg *m, struct file *f)
{
	char *field, *val;

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, &f->f_node->n_prot, f->f_mode, &field, &val) == 0) {
		return;
	}

	/*
	 * Clone off our home node?
	 */
	if (!strcmp(field, "fork")) {
		env_fork(f);
		m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Not a field we support...
	 */
	msg_err(m->m_sender, EINVAL);
}
@


1.4
log
@Convert %ud -> %u
@
text
@d7 1
a7 1
#include <env/env.h>
a10 1
#include <lib/llist.h>
@


1.3
log
@Add UID tag, get from client
@
text
@d48 1
a48 1
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%ud\n",
@


1.2
log
@Add a fork message, get rid of some old junk, tidy
up linked list traversal
@
text
@d48 2
a49 2
	sprintf(buf, "size=%d\ntype=%c\nowner=1/1\ninode=%ud\n",
		len, DIR(n) ? 'd' : 'f', n);
@


1.1
log
@Initial revision
@
text
@a6 1
#define _NAMER_H_INTERNAL
a22 1
	char buf2[8];
d38 1
a38 1
	if (n->n_internal) {
d40 1
a40 1
		l = n->n_elems.l_forw;
d43 1
a43 1
			l = l->l_forw;
d49 1
a49 1
		len, n->n_internal ? 'd' : 'f', n);
d59 46
d117 10
@
