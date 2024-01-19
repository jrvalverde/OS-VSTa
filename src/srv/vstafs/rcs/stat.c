head	1.8;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.7
	V1_1:1.7;
locks; strict;
comment	@ * @;


1.8
date	94.06.03.04.46.53;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.10.03.02.49.10;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.31.00.38.30;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.22.26.48;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.29.21.20.11;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.29.19.12.33;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.09.00.16;	author vandys;	state Exp;
branches;
next	;


desc
@stat message handling
@


1.8
log
@Remove old proto
@
text
@/*
 * stat.c
 *	Do the stat function
 *
 * We also lump the chmod/chown stuff here as well
 */
#include "vstafs.h"
#include "buf.h"
#include <sys/perm.h>
#include <stdio.h>
#include <std.h>

extern char *perm_print(struct prot *);

/*
 * vfs_stat()
 *	Do stat
 */
void
vfs_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];
	struct fs_file *fs;
	struct buf *b;
	uint len;
	char typec;

	/*
	 * Verify access
	 */
	if (!(f->f_perm & ACC_READ)) {
		msg_err(m->m_sender, EPERM);
		return;
	}
	fs = getfs(f->f_file, &b);
	if (!fs) {
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Calculate length
	 */
	if (fs->fs_type == FT_DIR) {
		ulong idx;

		typec = 'd';
		idx = sizeof(struct fs_file); 
		len = 0;
		lock_buf(b);
		while (idx < fs->fs_len) {
			uint ent_len;
			struct fs_dirent *d;

			/*
			 * End loop when can't get more
			 */
			if (!bmap(b, fs, idx, sizeof(struct fs_dirent),
					(char **)&d, &ent_len)) {
				break;
			}
			if (ent_len < sizeof(struct fs_dirent)) {
				break;
			}
			if ((d->fs_clstart != 0) && !(d->fs_name[0] & 0x80)) {
				len += 1;
			}
			idx += sizeof(struct fs_dirent);
		}
		unlock_buf(b);
	} else {
		typec = 'f';
		len = fs->fs_len - sizeof(struct fs_file);
	}
	sprintf(buf, "size=%u\ntype=%c\nowner=%d\ninode=%u\n",
		len, typec, fs->fs_owner, fs->fs_blks[0].a_start);
	strcat(buf, perm_print(&fs->fs_prot));
	m->m_buf = buf;
	m->m_arg = m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * vfs_wstat()
 *	Allow writing of supported stat messages
 */
void
vfs_wstat(struct msg *m, struct file *f)
{
	char *field, *val;
	struct fs_file *fs;

	/*
	 * Get file's node
	 */
	fs = getfs(f->f_file, 0);

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, &fs->fs_prot, f->f_perm, &field, &val) == 0)
		return;

	/*
	 * Not a field we support...
	 */
	msg_err(m->m_sender, EINVAL);
}

/*
 * vfs_fid()
 *	Return ID for file
 */
void
vfs_fid(struct msg *m, struct file *f)
{
	struct fs_file *fs;

	/*
	 * Only *files* get an ID (and thus can be mapped shared)
	 */
	fs = getfs(f->f_file, 0);
	if (fs->fs_type == FT_DIR) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * arg is the inode value; arg1 is the size in pages
	 */
	m->m_arg = fs->fs_blks[0].a_start;
	m->m_arg1 = btop(fs->fs_len);
	m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}
@


1.7
log
@Source reorg
@
text
@a13 1
extern int do_wstat(struct msg *, struct prot *, uint, char **, char **);
@


1.6
log
@Convert %ud -> %u
@
text
@d7 2
a8 2
#include <vstafs/vstafs.h>
#include <vstafs/buf.h>
@


1.5
log
@New arg to bmap(), missing unlock_buf().
@
text
@d27 1
d48 1
d73 1
d76 2
a77 3
	sprintf(buf, "size=%d\ntype=%c\nowner=%d\ninode=%ud\n",
		len, f->f_file ? 'f' : 'd', fs->fs_owner,
		fs->fs_blks[0].a_start);
@


1.4
log
@Clean up -Wall warnings
@
text
@d57 1
a57 1
			if (!bmap(fs, idx, sizeof(struct fs_dirent),
d69 1
@


1.3
log
@Add stat() functions, including vfs_fid.
@
text
@d10 2
d13 2
a14 1
extern char *perm_print();
@


1.2
log
@Get rid of unneeded fs_perms()
@
text
@d8 1
d21 1
a21 2
	uint len, owner;
	struct openfile *o;
d23 1
a23 1
	struct fs_file *fs;
d32 5
d41 4
a44 8
	o = f->f_file;
	if (!o) {
		struct llist *l;
		extern struct llist files;

		/*
		 * Root dir--# files in dir
		 */
d46 19
a64 2
		for (l = LL_NEXT(&files); l != &files; l = LL_NEXT(l)) {
			len += 1;
a65 1
		owner = 0;
d67 1
a67 5
		/*
		 * File--its byte length
		 */
		len = o->o_len;
		owner = o->o_owner;
d70 3
a72 2
		len, f->f_file ? 'f' : 'd', owner, o);
	strcat(buf, perm_print(&o->o_prot));
d88 1
d91 1
a91 1
	 * Can't fiddle the root dir
d93 1
a93 3
	if (f->f_file == 0) {
		msg_err(m->m_sender, EINVAL);
	}
d98 1
a98 1
	if (do_wstat(m, &f->f_file->o_prot, f->f_perm, &field, &val) == 0)
d105 27
@


1.1
log
@Initial revision
@
text
@a7 1
#include <sys/param.h>
a8 2
#include <sys/fs.h>
#include <lib/llist.h>
a12 14
 * fs_perms()
 *	Given the current open file, return access rights granted
 */
uint
fs_perms(struct perm *perms, uint nperm, struct openfile *o)
{
	struct fs_file *fs;
	struct buf *b;

	b = find_buf(o->
	fs = index_buf(
}

/*
d22 2
@
