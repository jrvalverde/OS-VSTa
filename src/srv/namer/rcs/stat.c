head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.4
date	93.11.16.02.48.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.10.03.02.49.10;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.27.11;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.12.19;	author vandys;	state Exp;
branches;
next	;


desc
@Stat handling
@


1.4
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
#define _NAMER_H_INTERNAL
#include <sys/namer.h>
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include <llist.h>

extern char *perm_print();

/*
 * namer_stat()
 *	Do stat
 */
void
namer_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];
	char buf2[8];
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
	if (n->n_internal) {
		len = 0;
		l = n->n_elems.l_forw;
		while (l != &n->n_elems) {
			len += 1;
			l = l->l_forw;
		}
	} else {
		sprintf(buf2, "%d", n->n_port);
		len = strlen(buf2);
	}
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%u\n",
		len, n->n_internal ? 'd' : 'f', n->n_owner, n);
	strcat(buf, perm_print(&n->n_prot));
	m->m_buf = buf;
	m->m_arg = m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * namer_wstat()
 *	Allow writing of supported stat messages
 */
void
namer_wstat(struct msg *m, struct file *f)
{
	char *field, *val;

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, &f->f_node->n_prot, f->f_mode, &field, &val) == 0)
		return;

	/*
	 * Not a field we support...
	 */
	msg_err(m->m_sender, EINVAL);
}
@


1.3
log
@Convert %ud -> %u
@
text
@d8 1
a8 1
#include <namer/namer.h>
d12 1
a12 1
#include <lib/llist.h>
@


1.2
log
@Add UID tag for nodes
@
text
@d51 1
a51 1
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%ud\n",
@


1.1
log
@Initial revision
@
text
@d51 2
a52 2
	sprintf(buf, "size=%d\ntype=%c\nowner=1/1\ninode=%ud\n",
		len, n->n_internal ? 'd' : 'f', n);
@
