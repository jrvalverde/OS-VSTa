head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.4
date	94.10.05.18.32.29;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.49.30;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.10.03.02.49.10;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.23.19.10.15;	author vandys;	state Exp;
branches;
next	;


desc
@stat/wstat handling
@


1.4
log
@Merge in Dave Hudson's bug fixes, especially "foo | less"
function.
@
text
@/*
 * stat.c
 *	Do the stat function
 */
#include "pipe.h"
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>

extern char *perm_print();

/*
 * pipe_stat()
 *	Do stat
 */
void
pipe_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];
	uint len, owner;
	struct pipe *o;
	struct llist *l;

	/*
	 * Verify access
	 */
	if (!(f->f_perm & ACC_READ)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Calculate length
	 */
	o = f->f_file;
	if (!o) {
		extern struct llist files;

		/*
		 * Root dir--# files in dir
		 */
		len = 0;
		for (l = LL_NEXT(&files); l != &files; l = LL_NEXT(l)) {
			len += 1;
		}
		owner = 0;
	} else {
		/*
		 * File--its byte length
		 */
		len = 0;
		for (l = LL_NEXT(&o->p_writers); l != &o->p_writers;
				l = LL_NEXT(l)) {
			uint y;
			struct msg *m2;

			m2 = l->l_data;
			for (y = 0; y < m2->m_nseg; ++y) {
				len += m2->m_seg[y].s_buflen;
			}
		}
		owner = o->p_owner;
	}
	sprintf(buf, "size=%d\ntype=%s\nowner=%d\ninode=%u\n",
		len, o ? "fifo" : "d", owner, o);
	if (o) {
		strcat(buf, perm_print(&o->p_prot));
	} else {
		sprintf(buf + strlen(buf), "perm=1\nacc=%d/%d\n",
			ACC_READ | ACC_WRITE, ACC_CHMOD);
	}
	m->m_buf = buf;
	m->m_arg = m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * pipe_wstat()
 *	Allow writing of supported stat messages
 */
void
pipe_wstat(struct msg *m, struct file *f)
{
	char *field, *val;

	/*
	 * Can't fiddle the root dir
	 */
	if (f->f_file == 0) {
		msg_err(m->m_sender, EINVAL);
	}

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, &f->f_file->p_prot, f->f_perm, &field, &val) == 0)
		return;

	/*
	 * Not a field we support...
	 */
	msg_err(m->m_sender, EINVAL);
}
@


1.3
log
@Source reorg
@
text
@d64 8
a71 3
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%u\n",
		len, f->f_file ? 'f' : 'd', owner, o);
	strcat(buf, perm_print(&o->p_prot));
@


1.2
log
@Convert %ud -> %u
@
text
@d5 1
a5 1
#include <pipe/pipe.h>
a8 1
#include <lib/llist.h>
@


1.1
log
@Initial revision
@
text
@d65 1
a65 1
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%ud\n",
@
