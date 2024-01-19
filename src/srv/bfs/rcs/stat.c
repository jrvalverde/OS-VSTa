head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	94.04.10.19.54.29;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.03.08.20.04.21;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.47.08;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.25.49;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.39.35;	author vandys;	state Exp;
branches;
next	;


desc
@Stat interface
@


1.5
log
@Cleanup, add time stamp support, add rename() support
@
text
@/*
 * Filename:	stat.c
 * Developed:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Originated:	Andy Valencia
 * Last Update: 20th February 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description: Implement stat operations on an open file
 */


#include <std.h>
#include <stdio.h>
#include <sys/param.h>
#include "bfs.h"


extern struct super *sblock;


/*
 * bfs_stat()
 *	Build stat string for file, send back
 */
void
bfs_stat(struct msg *m, struct file *f)
{
	char result[MAXSTAT];
	struct inode *i;

	i = f->f_inode;
	sprintf(result,
	 	"perm=1/1\nacc=5/0/2\nsize=%d\ntype=%c\nowner=0\n" \
	 	"inode=%d\nctime=%u\nmtime=%u",
		i->i_fsize, (i->i_num == ROOTINODE) ? 'd' : 'f',
		i->i_num, i->i_ctime, i->i_mtime);
	sprintf(&result[strlen(result)],
		"\nstart blk=%d\nmgd blks=%d\ni_refs=%d\n",
		i->i_start, i->i_blocks, i->i_refs);
	sprintf(&result[strlen(result)],
		"prev=%d\nnext=%d\n",
		i->i_prev, i->i_next);

	m->m_buf = result;
	m->m_buflen = strlen(result);
	m->m_nseg = 1;
	m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}
@


1.4
log
@Rev boot filesystem per work from Dave Hudson
@
text
@d33 2
a34 1
	 	"perm=1/1\nacc=5/0/2\nsize=%d\ntype=%c\nowner=0\ninode=%d\n",
d36 1
a36 1
		i->i_num);
d38 1
a38 1
		"start blk=%d\nmgd blks=%d\ni_refs=%d\n",
@


1.3
log
@Source reorg
@
text
@d2 7
a8 2
 * stat.c
 *	Implement stat operations on an open file
d10 5
d16 3
a18 1
#include <sys/param.h>
a19 1
extern char *strerror();
d29 13
a41 1
	struct dirent d;
a42 19
	/*
	 * Root is hard-coded
	 */
	if (f->f_inode == ROOTINO) {
		sprintf(result,
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=d\nowner=0\ninode=-1\n",
			NDIRBLOCKS*BLOCKSIZE);
	} else {
		/*
		 * Otherwise look up file and get dope
		 */
		if (dir_copy(f->f_inode->i_num, &d)) {
			msg_err(m->m_sender, strerror());
			return;
		}
		sprintf(result,
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=f\nowner=0\ninode=%d\n",
			d.d_len, d.d_inum);
	}
@


1.2
log
@new owner code
@
text
@d5 1
a5 1
#include <bfs/bfs.h>
@


1.1
log
@Initial revision
@
text
@d25 1
a25 1
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=d\nowner=1/1\ninode=-1\n",
d36 1
a36 1
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=f\nowner=1/1\ninode=%d\n",
@
