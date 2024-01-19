head	1.6;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5
	V1_1:1.5
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.6
date	94.03.08.20.04.21;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.47.08;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.06.30.19.52.24;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.19.14.57.19;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.10.20.31.34;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.39.30;	author vandys;	state Exp;
branches;
next	;


desc
@Read/write interface
@


1.6
log
@Rev boot filesystem per work from Dave Hudson
@
text
@/*
 * Filename:	rw.c
 * Developed:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Originated:	Andy Valencia
 * Last Update: 11th February 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description: Routines for operating on the data in a file
 */


#include <std.h>
#include <stdio.h>
#include <sys/param.h>
#include "bfs.h"


extern struct super *sblock;


/*
 * do_write()
 *	Local routine to loop over a buffer and write it to a file
 *
 * Returns 0 on success, 1 on error.
 */
static int
do_write(int startblk, int pos, char *buf, int cnt)
{
	int x, step, blk, boff;
	void *handle;

	/*
	 * Loop across each block, putting our data into place
	 */
	for (x = 0; x < cnt; ) {
		/*
		 * Calculate how much to take out of current block
		 */
		boff = pos & (BLOCKSIZE - 1);
		step = BLOCKSIZE - boff;
		if (step >= cnt) {
			step = cnt;
		}

		/*
		 * Map current block
		 */
		blk = pos / BLOCKSIZE;
		handle = bget(startblk+blk);
		if (!handle)
			return 1;
		memcpy((char *)bdata(handle) + boff, buf + x, step);
		pos += step;
		bdirty(handle);
		bfree(handle);

		/*
		 * Advance to next chunk
		 */
		x += step;
	}
	return(0);
}


/*
 * bfs_write()
 *	Write to an open file
 */
void
bfs_write(struct msg *m, struct file *f)
{
	struct inode *i = f->f_inode;

	/*
	 * Can only write to a true file, and only if open for writing.
	 */
	if ((i->i_num == ROOTINODE) || !f->f_write) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * See if the file's going to be able to hold all the data.  We
	 * do not necessarily need to allocate space if we're rewriting
	 * an existing file.
	 */
	if ((f->f_pos + m->m_buflen) > i->i_fsize) {
		if (blk_alloc(i, f->f_pos + m->m_buflen)) {
			msg_err(m->m_sender, ENOSPC);
			return;
		}
	}

	/*
	 * Copy out the buffer
	 */
	if (do_write(i->i_start, f->f_pos, m->m_buf, m->m_buflen)) {
		msg_err(m->m_sender, strerror());
		return;
	}
	m->m_arg = m->m_buflen;
	f->f_pos += m->m_buflen;
	m->m_buflen = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}


/*
 * bfs_readdir()
 *	Do reads on directory entries
 */
static void
bfs_readdir(struct msg *m, struct file *f)
{
	struct inode *i;
	char *buf;
	int x, len, err, ok = 1;

	/*
	 * Make sure it's the root directory
	 */
	if (f->f_inode->i_num != ROOTINODE) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Get a buffer of the requested size, but put a sanity
	 * cap on it.
	 */
	len = m->m_arg;
	if (len > 256) {
		len = 256;
	}
	if ((buf = malloc(len + 1)) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Assemble as many names as will fit
	 */
	for (x = 0; x < len; ) {
		/*
		 * Find next directory entry.  Null name means
		 * it's an empty slot.
		 */
		while (((err = (f->f_pos >= sblock->s_ndirents)) == 0)
			&& ok) {
			i = ino_find(f->f_pos);
			if ((i != NULL) && (i->i_name[0] != '\0')) {
				ok = 0;
			} else {
				f->f_pos += 1;
			}
		}
		ok = 1;

		/*
		 * If error or EOF, return what we have
		 */
		if (err) {
			break;
		}

		/*
		 * If the next entry won't fit, back up the file
		 * position and return what we have.
		 */
		if ((x + strlen(i->i_name) + 1) >= len) {
			break;
		}

		/*
		 * Add entry and a newline
		 */
		strcat(buf + x, i->i_name);
		strcat(buf + x, "\n");
		x += (strlen(i->i_name) + 1);
		f->f_pos += 1;
	}

	/*
	 * Send back results
	 */
	m->m_buf = buf;
	m->m_arg = m->m_buflen = x;
	m->m_nseg = ((x > 0) ? 1 : 0);
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	free(buf);
}


/*
 * bfs_read()
 *	Read bytes out of the current file or directory
 *
 * Directories get their own routine.
 */
void
bfs_read(struct msg *m, struct file *f)
{
	int x, step, cnt, blk, boff;
	struct inode *i;
	void *handle;
	char *buf;

	/*
	 * Directory--only one is the root
	 */
	if (f->f_inode->i_num == ROOTINODE) {
		bfs_readdir(m, f);
		return;
	}

	i = f->f_inode;

	/*
	 * EOF?
	 */
	if (f->f_pos >= i->i_fsize) {
		m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Calculate # bytes to get
	 */
	cnt = m->m_arg;
	if (cnt > (i->i_fsize - f->f_pos)) {
		cnt = i->i_fsize - f->f_pos;
	}

	/*
	 * Get a buffer big enough to do the job
	 */
	buf = malloc(cnt);
	if (buf == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Loop across each block, putting our data into place
	 */
	for (x = 0; x < cnt; ) {
		/*
		 * Calculate how much to take out of current block
		 */
		boff = f->f_pos & (BLOCKSIZE - 1);
		step = BLOCKSIZE - boff;
		if (step >= cnt) {
			step = cnt;
		}

		/*
		 * Map current block
		 */
		blk = f->f_pos / BLOCKSIZE;
		handle = bget(i->i_start + blk);
		if (!handle) {
			free(buf);
			msg_err(m->m_sender, strerror());
			return;
		}
		memcpy(buf + x, (char *)bdata(handle) + boff, step);
		f->f_pos += step;
		bfree(handle);

		/*
		 * Advance to next chunk
		 */
		x += step;
	}

	/*
	 * Send back reply
	 */
	m->m_buf = buf;
	m->m_arg = m->m_buflen = cnt;
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	free(buf);
}
@


1.5
log
@Source reorg
@
text
@d2 7
a8 2
 * rw.c
 *	Routines for operating on the data in a file
d10 4
a16 2
extern void *bget(), *malloc(), *bdata();
extern char *strerror();
d18 3
d27 1
a27 1
static
d40 1
a40 1
		boff = pos & (BLOCKSIZE-1);
d53 1
a53 1
		memcpy((char *)bdata(handle)+boff, buf+x, step);
d66 1
a73 2
	void *handle;
	struct dirent d;
d79 1
a79 1
	if ((i == ROOTINO) || !f->f_write) {
a84 11
	 * Get a picture of what the directory entry currently
	 * looks like.  The d_len field will change after new
	 * blocks are allocated, but d_start will remain the
	 * same.
	 */
	if (dir_copy(i->i_num, &d)) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
d89 1
a89 1
	if ((f->f_pos + m->m_buflen) > d.d_len) {
d99 1
a99 1
	if (do_write(d.d_start, f->f_pos, m->m_buf, m->m_buflen)) {
d109 1
d117 1
a117 1
	struct dirent d;
d119 1
a119 1
	int x, len, err;
d124 1
a124 1
	if (f->f_inode != ROOTINO) {
d137 1
a137 1
	if ((buf = malloc(len+1)) == 0) {
d150 8
a157 3
		while (((err = dir_copy(f->f_pos, &d)) == 0) &&
				(d.d_name[0] == '\0')) {
			f->f_pos += 1;
d159 1
d172 1
a172 1
		if ((x + strlen(d.d_name) + 1) >= len) {
d179 3
a181 3
		strcat(buf+x, d.d_name);
		strcat(buf+x, "\n");
		x += (strlen(d.d_name)+1);
d196 1
a208 1
	struct dirent d;
d214 1
a214 1
	if (f->f_inode == ROOTINO) {
a218 4
	/*
	 * Get a snapshot of our dir entry.  It can't change (we're
	 * single-threaded), and we won't be modifying it.
	 */
a219 4
	if (dir_copy(i->i_num, &d)) {
		msg_err(m->m_sender, strerror());
		return;
	}
d224 1
a224 1
	if (f->f_pos >= d.d_len) {
d234 2
a235 2
	if (cnt > (d.d_len - f->f_pos)) {
		cnt = d.d_len - f->f_pos;
d254 1
a254 1
		boff = f->f_pos & (BLOCKSIZE-1);
d264 1
a264 1
		handle = bget(d.d_start+blk);
d270 1
a270 1
		memcpy(buf+x, (char *)bdata(handle)+boff, step);
@


1.4
log
@GCC warning cleanup
@
text
@d6 1
a6 1
#include <bfs/bfs.h>
@


1.3
log
@Fix file offset handling
@
text
@d8 1
a8 1
extern void *bget(), *malloc();
d43 1
a43 1
		memcpy(bdata(handle)+boff, buf+x, step);
d273 1
a273 1
		memcpy(buf+x, bdata(handle)+boff, step);
@


1.2
log
@Get seg counts right, especially for EOF.  Drop bogus bfree().
I systematically forgot to update file position.
@
text
@d44 1
d274 1
a291 1
	f->f_pos += cnt;
@


1.1
log
@Initial revision
@
text
@d100 1
a100 1
	if (do_write(d.d_start, f->f_pos, m->m_buf, m->m_buflen) {
d105 1
d184 1
a184 1
	m->m_nseg = 1;
a226 1
		bfree(handle);
d290 1
@
