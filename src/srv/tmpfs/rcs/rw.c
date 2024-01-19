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
date	93.11.16.02.49.42;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.02.23.58.01;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.21.22.36.40;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.12.20.51.08;	author vandys;	state Exp;
branches;
next	;


desc
@Reading/writing of files
@


1.4
log
@Source reorg
@
text
@/*
 * rw.c
 *	Routines for operating on the data in a file
 */
#include "tmpfs.h"
#include <hash.h>
#include <llist.h>
#include <std.h>

/*
 * do_write()
 *	Local routine to loop over a buffer and write it to a file
 *
 * Returns 0 on success, 1 on error.
 */
static
do_write(struct openfile *o, int pos, char *buf, int cnt)
{
	uint x, step, blk, boff;
	char *blkp;

	/*
	 * Loop across each block, putting our data into place
	 */
	for (x = 0; x < cnt; x += step) {
		/*
		 * Calculate how much to take out of current block
		 */
		boff = pos & (BLOCKSIZE-1);
		step = BLOCKSIZE - boff;
		if (step >= (cnt - x)) {
			step = (cnt - x);
		}

		/*
		 * Get next block.  If not present yet, allocate
		 * and add to the file.
		 */
		blk = pos / BLOCKSIZE;
		blkp = hash_lookup(o->o_blocks, blk);
		if (blkp == 0) {
			blkp = malloc(BLOCKSIZE);
			if (blkp == 0) {
				return(1);
			}
			bzero(blkp, BLOCKSIZE);
			if (hash_insert(o->o_blocks, blk, blkp)) {
				free(blkp);
				return(1);
			}
		}

		/*
		 * Put contents into block
		 */
		memcpy(blkp+boff, buf+x, step);

		/*
		 * Advance to next chunk
		 */
		pos += step;
	}
	return(0);
}

/*
 * tmpfs_write()
 *	Write to an open file
 */
void
tmpfs_write(struct msg *m, struct file *f)
{
	struct openfile *o = f->f_file;
	uint x, cnt, err;

	/*
	 * Can only write to a true file, and only if open for writing.
	 */
	if (!o || !(f->f_perm & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Walk each segment of the message
	 */
	err = cnt = 0;
	for (x = 0; x < m->m_nseg; ++x) {
		seg_t *s;

		/*
		 * Write it
		 */
		s = &m->m_seg[x];
		if (do_write(o, f->f_pos, s->s_buf, s->s_buflen)) {
			err = 1;
			break;
		}

		/*
		 * Update position and count
		 */
		f->f_pos += s->s_buflen;
		cnt += s->s_buflen;

		/*
		 * If we've extended the file, update the
		 * file length.
		 */
		if (f->f_pos > o->o_len) {
			o->o_len = f->f_pos;
		}
	}

	/*
	 * Done.  Return count of stuff written.
	 */
	if ((cnt == 0) && err) {
		msg_err(m->m_sender, strerror());
	} else {
		m->m_arg = cnt;
		m->m_arg1 = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
	}
}

/*
 * tmpfs_readdir()
 *	Do reads on directory entries
 */
static void
tmpfs_readdir(struct msg *m, struct file *f)
{
	char *buf;
	uint len, pos, bufcnt;
	struct llist *l;
	struct openfile *o;
	extern struct llist files;

	/*
	 * Get a buffer of the requested size, but put a sanity
	 * cap on it.
	 */
	len = m->m_arg;
	if (len > 256) {
		len = 256;
	}
	if ((buf = malloc(len+1)) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}
	buf[0] = '\0';

	/*
	 * Assemble as many names as will fit, starting at
	 * given byte offset.  We assume the caller's position
	 * always advances in units of a whole directory entry.
	 */
	bufcnt = pos = 0;
	for (l = LL_NEXT(&files); l != &files; l = LL_NEXT(l)) {
		uint slen;

		/*
		 * Point to next file.  Get its length.
		 */
		o = l->l_data;
		slen = strlen(o->o_name)+1;

		/*
		 * If we've reached an offset the caller hasn't seen
		 * yet, assemble the entry into the buffer.
		 */
		if (pos >= f->f_pos) {
			/*
			 * No more room in buffer--return results
			 */
			if (slen >= len) {
				break;
			}

			/*
			 * Put string with newline at end of buffer
			 */
			sprintf(buf + bufcnt, "%s\n", o->o_name);

			/*
			 * Update counters
			 */
			len -= slen;
			bufcnt += slen;
		}

		/*
		 * Update position
		 */
		pos += slen;
	}

	/*
	 * Send back results
	 */
	m->m_buf = buf;
	m->m_arg = m->m_buflen = bufcnt;
	m->m_nseg = ((bufcnt > 0) ? 1 : 0);
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	free(buf);
	f->f_pos = pos;
}

/*
 * tmpfs_read()
 *	Read bytes out of the current file or directory
 *
 * Directories get their own routine.
 */
void
tmpfs_read(struct msg *m, struct file *f)
{
	uint nseg, cnt, lim, blk;
	struct openfile *o;
	char *blkp;

	/*
	 * Directory--only one is the root
	 */
	if ((o = f->f_file) == 0) {
		tmpfs_readdir(m, f);
		return;
	}

	/*
	 * Access?
	 */
	if (!(f->f_perm & ACC_READ)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * EOF?
	 */
	if (f->f_pos >= o->o_len) {
		m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Calculate # bytes to get
	 */
	lim = m->m_arg;
	if (lim > (o->o_len - f->f_pos)) {
		lim = o->o_len - f->f_pos;
	}

	/*
	 * Build message segments
	 */
	cnt = 0;
	blk = f->f_pos / BLOCKSIZE;
	for (nseg = 0; (cnt < lim) && (nseg < MSGSEGS); ++nseg) {
		uint off, sz;

		/*
		 * Get next block of data.  We simulate sparse
		 * files by using our pre-allocated source of
		 * zeroes.
		 */
		blkp = hash_lookup(o->o_blocks, blk);
		if (blkp == 0) {
			extern char *zeroes;

			blkp = zeroes;
		}

		/*
		 * Calculate how much of the block to add to
		 * the message.
		 */
		off = f->f_pos & (BLOCKSIZE-1);
		sz = BLOCKSIZE-off;
		if ((cnt+sz) > lim) {
			sz = lim-cnt;
		}

		/*
		 * Put into next message segment
		 */
		m->m_seg[nseg].s_buf = blkp+off;
		m->m_seg[nseg].s_buflen = sz;

		/*
		 * Advance counter
		 */
		cnt += sz;
		f->f_pos += sz;
		blk += 1;
	}

	/*
	 * Send back reply
	 */
	m->m_arg = cnt;
	m->m_nseg = nseg;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}
@


1.3
log
@Fix math when doing more than a BLOCKSIZE at a time
@
text
@d5 3
a7 3
#include <tmpfs/tmpfs.h>
#include <lib/hash.h>
#include <lib/llist.h>
@


1.2
log
@Didn't use the adjusted limit value for buffer size
@
text
@d31 2
a32 2
		if (step >= cnt) {
			step = cnt;
@


1.1
log
@Initial revision
@
text
@d291 1
a291 1
		m->m_seg[nseg].s_buflen = BLOCKSIZE-off;
@
