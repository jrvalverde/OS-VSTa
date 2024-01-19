head	1.6;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.6
date	94.11.16.19.37.51;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.05.21.21.45.35;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.49.42;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.10.03.02.49.10;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.28.29;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.12.20.51.08;	author vandys;	state Exp;
branches;
next	;


desc
@stat/wstat handling
@


1.6
log
@Add FS_FID support so we can run a.out's from /tmp
@
text
@/*
 * stat.c
 *	Do the stat function
 *
 * We also lump the chmod/chown stuff here as well
 */
#include "tmpfs.h"
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include <llist.h>

extern char *perm_print();

/*
 * tmpfs_stat()
 *	Do stat
 */
void
tmpfs_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];
	uint len, owner;
	struct openfile *o;

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
		struct llist *l;
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
		len = o->o_len;
		owner = o->o_owner;
	}
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%u\n",
		len, f->f_file ? 'f' : 'd', owner, o);
	if (o) {
		strcat(buf, perm_print(&o->o_prot));
	} else {
		sprintf(buf+strlen(buf), "perm=1\nacc=%d/%d\n",
			ACC_READ | ACC_WRITE, ACC_CHMOD);
	}
	m->m_buf = buf;
	m->m_arg = m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * tmpfs_wstat()
 *	Allow writing of supported stat messages
 */
void
tmpfs_wstat(struct msg *m, struct file *f)
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
	if (do_wstat(m, &f->f_file->o_prot, f->f_perm, &field, &val) == 0)
		return;

	/*
	 * Not a field we support...
	 */
	msg_err(m->m_sender, EINVAL);
}

/*
 * tmpfs_fid()
 *	Return file ID/size for kernel caching of mappings
 */
void
tmpfs_fid(struct msg *m, struct file *f)
{
	struct openfile *o = f->f_file;

	/*
	 * Only files get an ID
	 */
	if (o == 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	m->m_arg = (ulong)o;
	m->m_arg1 = o->o_len;
	m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}
@


1.5
log
@don't use null pointer
@
text
@d99 22
@


1.4
log
@Source reorg
@
text
@d59 6
a64 1
	strcat(buf, perm_print(&o->o_prot));
@


1.3
log
@Convert %ud -> %u
@
text
@d7 1
a7 1
#include <tmpfs/tmpfs.h>
d11 1
a11 1
#include <lib/llist.h>
@


1.2
log
@Add UID tag for nodes, get from client
@
text
@d57 1
a57 1
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%ud\n",
@


1.1
log
@Initial revision
@
text
@d23 1
a23 1
	int len;
d49 1
d55 1
d57 2
a58 2
	sprintf(buf, "size=%d\ntype=%c\nowner=1/1\ninode=%ud\n",
		len, f->f_file ? 'd' : 'f', o);
@
