head	1.19;
access;
symbols
	V1_3_1:1.19
	V1_3:1.19
	V1_2:1.18
	V1_1:1.17;
locks; strict;
comment	@ * @;


1.19
date	94.02.28.22.06.05;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.11.19.04.26.19;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.10.23.20.50.03;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.09.27.23.10.03;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.09.27.18.27.32;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.09.19.19.15.23;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.09.18.18.09.45;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.09.18.00.08.57;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.09.15.01.27.40;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.09.11.19.09.40;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.09.11.19.05.46;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.08.31.03.06.14;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.31.00.39.14;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.30.21.36.56;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.22.26.48;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.29.19.12.00;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.27.13.42.14;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.09.00.02;	author vandys;	state Exp;
branches;
next	;


desc
@Reading/writing/extending files
@


1.19
log
@Convert to syslog()
@
text
@/*
 * rw.c
 *	Routines for operating on the data in a file
 *
 * The storage allocation philosophy used is to add EXTSIZ chunks of data
 * to the file when possible, and mark the file's length as such.  On close,
 * the trailing part of the file is truncated and the file's true length
 * updated.  This technique also means that after a crash the file's header
 * accurately describes the data associated with the file.
 */
#include "vstafs.h"
#include "alloc.h"
#include "buf.h"
#include <hash.h>
#include <std.h>
#include <sys/assert.h>

/*
 * file_grow()
 *	Extend the block allocation to reach the indicated size
 *
 * newsize is in sectors.  Returns 0 on success, 1 on failure.
 */
static int
file_grow(struct buf *b_fs, struct fs_file *fs, ulong newsize)
{
	ulong incr, got;
	struct alloc *a;
	daddr_t from, newstart;

	/*
	 * Find extent containing this position
	 */
	ASSERT_DEBUG(fs->fs_nblk > 0, "file_grow: no extents");
	a = &fs->fs_blks[fs->fs_nblk-1];

	/*
	 * Extend contiguously as far as possible.
	 */
	from = btors(fs->fs_len);
	ASSERT_DEBUG(newsize > from, "file_grow: shrink");
	incr = MAX(newsize - from, EXTSIZ);
	newstart = a->a_start + a->a_len;
	got = take_block(newstart, incr);
	if (got > 0) {
		ulong buflen;

		/*
		 * Calculate new length of this buffer
		 */
		buflen = MIN((a->a_len & (EXTSIZ-1)) + got, EXTSIZ);

		/*
		 * Tell buffer management about this incremental space
		 */
		if (resize_buf(a->a_start + (a->a_len & ~(EXTSIZ-1)),
				buflen, 0)) {
			free_block(newstart, got);
			return(1);
		}
		fs = index_buf(b_fs, 0, 1);
		a = &fs->fs_blks[fs->fs_nblk-1];

		/*
		 * Update extent information
		 */
		a->a_len += got;
		from += got;
		fs->fs_len = stob(from);
	}

	/*
	 * If we've gotten at least the required amount, return success
	 * now.
	 */
	if (from >= newsize) {
		return(0);
	}

	/*
	 * Fail if there's no more slots for extents
	 */
	if (fs->fs_nblk >= MAXEXT) {
		return(1);
	}

	/*
	 * Sigh.  Another extent gets eaten.  Grab a full EXTSIZ chunk.
	 * We could quibble about shrinking this, but if you're down to
	 * your last 64K contiguous space, it's seriously time to defrag
	 * your disk.
	 */
	a += 1;
	a->a_start = alloc_block(EXTSIZ);
	if (a->a_start == 0) {
		return(1);
	}

	/*
	 * Mark the storage in our fs_file, and return success
	 */
	a->a_len = EXTSIZ;
	fs->fs_nblk += 1;
	fs->fs_len = stob(from+EXTSIZ);
	return(0);
}

/*
 * bmap()
 *	Access the right buffer for the given data
 *
 * If necessary, grow the file.  Figure out which buffer contains
 * the data, and map this buffer in.
 *
 * We return 0 on error, otherwise the struct buf corresponding to
 * the returned data range.  On success, *blkp points to the appropriate
 * spot within the buffer, and *stepp holds the number of bytes available
 * at this location.
 *
 * bmap() may return space which is physically within the file, but
 * currently beyond fs_len.
 */
struct buf *
bmap(struct buf *b_fs, struct fs_file *fs, ulong pos,
	uint cnt, char **blkp, uint *stepp)
{
	struct alloc *a;
	uint x;
	ulong osize, nsize, extoff, len;
	daddr_t extstart, start;
	struct buf *b;

	/*
	 * Grow file if needed
	 */
	if (pos >= fs->fs_len) {
		/*
		 * Calculate growth.  If more blocks are needed, get
		 * them now.  Otherwise just fiddle the file length.
		 */
		osize = btors(fs->fs_len);
		nsize = btors(pos + cnt);
		if (nsize > osize) {
			if (file_grow(b_fs, fs, nsize)) {
				return(0);
			}
			fs = index_buf(b_fs, 0, 1);
		} else {
			fs->fs_len = pos+cnt;
		}
		dirty_buf(b_fs);
		sync_buf(b_fs);
	}

	/*
	 * Find the appropriate extent.  As we go, update "extoff"
	 * so that when we find the right extent we will then
	 * know how far into the extent our data exists.
	 */
	a = &fs->fs_blks[0];
	extoff = nsize = btos(pos);
	osize = 0;
	for (x = 0; x < fs->fs_nblk; ++x,++a) {
		osize += a->a_len;
		if (nsize < osize) {
			break;
		}
		extoff -= a->a_len;
	}
	ASSERT_DEBUG(x < fs->fs_nblk, "bmap: no extent");

	/*
	 * Find the appropriate EXTSIZ part of the extent to use, since
	 * our buffer pool operates on chunks of EXTSIZ in length.
	 * nsize holds the absolute sector index of the desired position.
	 * extoff holds the number of sectors into the current extent
	 * where the requested data starts.
	 */
	extstart = (extoff & ~(EXTSIZ-1));
	start = a->a_start + extstart;
	len = MIN(a->a_len - extstart, EXTSIZ);
	b = find_buf(start, len);
	if (b == 0) {
		return(0);
	}

	/*
	 * Map in the part we need, fill in its location and length
	 */
	len = len - (extoff & (EXTSIZ-1));
	*blkp = index_buf(b, extoff & (EXTSIZ-1), len);
	if (*blkp == 0) {
		return(0);
	}
	*blkp += (pos % SECSZ);
	x = stob(len) - (pos % SECSZ);
	if (x > cnt) {
		*stepp = cnt;
	} else {
		*stepp = x;
	}
	return(b);
}

/*
 * do_write()
 *	Local routine to loop over a buffer and write it to a file
 *
 * Returns 0 on success, 1 on error.
 */
static int
do_write(struct openfile *o, ulong pos, char *buf, uint cnt)
{
	struct fs_file *fs;
	struct buf *b;
	int result = 0;

	/*
	 * Get access to file structure information, which resides in the
	 * first sector of the file.
	 */
	fs = getfs(o, &b);
	if (!fs) {
		return(1);
	}
	lock_buf(b);

	/*
	 * Loop across each block, putting our data into place
	 */
	while (cnt > 0) {
		struct buf *b2;
		uint step;
		char *blkp;

		/*
		 * Find appropriate extent
		 */
		b2 = bmap(b, fs, pos, cnt, &blkp, &step);
		fs = index_buf(b, 0, 1);
		if (!b2) {
			result = 1;
			break;
		}

		/*
		 * Mirror the size of the first extent, so we can map
		 * it correctly out of the cache.
		 */
		o->o_len = fs->fs_blks[0].a_len;

		/*
		 * Cap at amount of I/O to do here
		 */
		step = MIN(step, cnt);

		/*
		 * Put contents into block, mark buffer modified
		 */
		bcopy(buf, blkp, step);
		dirty_buf(b2);

		/*
		 * Advance to next chunk
		 */
		pos += step;
		buf += step;
		cnt -= step;
	}

	/*
	 * This needs to stay up-to-date so we can pull the correct size
	 * out of the pool.
	 */
	o->o_len = fs->fs_blks[0].a_len;

	/*
	 * Free leading buf, return result
	 */
	unlock_buf(b);
	return(result);
}

/*
 * vfs_write()
 *	Write to an open file
 */
void
vfs_write(struct msg *m, struct file *f)
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
		if (f->f_pos > o->o_hiwrite) {
			o->o_hiwrite = f->f_pos;
		}
		cnt += s->s_buflen;
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
 * vfs_readdir()
 *	Do reads on directory entries
 */
static void
vfs_readdir(struct msg *m, struct file *f)
{
	char *buf;
	uint len, bufcnt;
	struct buf *b;
	struct fs_file *fs;
	uint step;

	/*
	 * Get a buffer of the requested size, but put a sanity
	 * cap on it.
	 */
	len = MIN(m->m_arg, 256);
	if ((buf = malloc(len+1)) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}
	buf[0] = '\0';

	/*
	 * Map in the directory's structure
	 */
	fs = getfs(f->f_file, &b);
	if (!fs) {
		msg_err(m->m_sender, EIO);
		return;
	}
	lock_buf(b);

	/*
	 * Assemble as many names as will fit, starting at
	 * given byte offset.
	 */
	bufcnt = 0;
	step = 0;
	for (;;) {
		struct fs_dirent *d;
		uint slen;

		/*
		 * Map in next run of entries if need more
		 */
		if (step < MAXNAMLEN) {
			char *p;
			uint x;

			/*
			 * End if EOF or no room for another name
			 */
			x = roundup(len-bufcnt, MAXNAMLEN);
			ASSERT_DEBUG(f->f_pos <= fs->fs_len,
				"vfs_readdir: pos > len");
			x = MIN((x / MAXNAMLEN) * sizeof(struct fs_dirent),
				fs->fs_len - f->f_pos);
			if (x < 1) {
				break;
			}
			if (!bmap(b, fs, f->f_pos, x, &p, &step)) {
				break;
			}
			ASSERT_DEBUG(step >= sizeof(*d),
				"vfs_readdir: bad size");
			d = (struct fs_dirent *)p;
		}

		/*
		 * Skip deleted, bail when there are no more entries
		 */
		if (d->fs_clstart == 0) {
			break;
		}
		if ((d->fs_name[0] & 0x80) == 0) {
			/*
			 * Check that it'll fit.  Leave loop when it doesn't.
			 */
			slen = strlen(d->fs_name)+1;
			if ((bufcnt + slen) >= len) {
				break;
			}

			/*
			 * Add name and update counters
			 */
			strcat(buf + bufcnt, d->fs_name);
			strcat(buf + bufcnt, "\n");
			bufcnt += slen;
		}
		f->f_pos += sizeof(struct fs_dirent);
		step -= sizeof(struct fs_dirent);
		++d;
	}

	/*
	 * Done with dir, allow contents to be reclaimed
	 */
	unlock_buf(b);

	/*
	 * Send back results
	 */
	m->m_buf = buf;
	m->m_arg = m->m_buflen = bufcnt;
	m->m_nseg = ((bufcnt > 0) ? 1 : 0);
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	free(buf);
}

/*
 * vfs_read()
 *	Read bytes out of the current file or directory
 *
 * Directories get their own routine.
 */
void
vfs_read(struct msg *m, struct file *f)
{
	struct buf *b, *bufs[MSGSEGS];
	struct fs_file *fs;
	uint lim, cnt, nseg;

	/*
	 * Map in file structure info
	 */
	fs = getfs(f->f_file, &b);
	if (!fs) {
		msg_err(m->m_sender, EIO);
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
	 * Directory--only one is the root
	 */
	if (fs->fs_type == FT_DIR) {
		vfs_readdir(m, f);
		return;
	}

	/*
	 * EOF?
	 */
	if (f->f_pos >= fs->fs_len) {
		m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Hold fs_file until we're all done
	 */
	lock_buf(b);

	/*
	 * Calculate # bytes to get
	 */
	lim = m->m_arg;
	if (lim > (fs->fs_len - f->f_pos)) {
		lim = fs->fs_len - f->f_pos;
	}

	/*
	 * Build message segments
	 */
	cnt = 0;
	for (nseg = 0; (cnt < lim) && (nseg < MSGSEGS); ++nseg) {
		uint sz;
		struct buf *b2;
		char *p;

		/*
		 * Get next block of data
		 */
		b2 = bufs[nseg] = bmap(b, fs, f->f_pos, lim-cnt, &p, &sz);
		if (b2 == 0) {
			break;
		}
		if (b2 != b) {
			lock_buf(b2);
		}

		/*
		 * Put into next message segment
		 */
		m->m_seg[nseg].s_buf = p;
		m->m_seg[nseg].s_buflen = sz;

		/*
		 * Advance counter
		 */
		cnt += sz;
		f->f_pos += sz;
	}

	/*
	 * Send back reply
	 */
	m->m_arg = cnt;
	m->m_nseg = nseg;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);

	/*
	 * Free up bufs
	 */
	for (cnt = 0; cnt < nseg; ++cnt) {
		struct buf *b2;

		if ((b2 = bufs[cnt]) != b) {
			unlock_buf(b2);
		}
	}
	unlock_buf(b);
}
@


1.18
log
@Cap read of dir at current len; bmap will give us more
@
text
@a16 6
#ifdef DEBUG
#undef TRACE	/* Trace actions in bmap() */
#ifdef TRACE
#include <stdio.h>
#endif
#endif
a132 3
#ifdef TRACE
	printf("bmap pos %ld cnt %d fs_len %ld\n", pos, cnt, fs->fs_len);
#endif
a136 3
#ifdef TRACE
		printf(" grow to %ld\n", pos+cnt);
#endif
a169 3
#ifdef TRACE
	printf(" data in extent %d, extent off %ld\n", x, extoff);
#endif
a181 3
#ifdef TRACE
	printf(" start blk %ld len %ld\n", start, len);
#endif
a201 3
#ifdef TRACE
	printf(" avail %ld taken %d\n", x, *stepp);
#endif
@


1.17
log
@Source reorg
@
text
@d125 3
d416 5
a420 2
			x = (x / MAXNAMLEN) * sizeof(struct fs_dirent);
			if ((f->f_pos >= fs->fs_len) || (x < 1)) {
@


1.16
log
@Don't confuse space left for names with the count sent in to
bmap() to get the next fs_dirent.  In tracing this, add TRACE
code to watch bmap() crank.
@
text
@d11 4
a14 5
#include <vstafs/vstafs.h>
#include <vstafs/alloc.h>
#include <vstafs/buf.h>
#include <lib/hash.h>
#include <lib/llist.h>
@


1.15
log
@Track highest position written
@
text
@d18 6
d137 3
d144 3
d180 3
d195 3
d218 3
d406 1
a406 1
		if (step < sizeof(struct fs_dirent)) {
d408 1
d410 6
a415 1
			if (f->f_pos >= fs->fs_len) {
d418 1
a418 1
			if (!bmap(b, fs, f->f_pos, len-bufcnt, &p, &step)) {
@


1.14
log
@Add stuff to protect against buffer moving on resize
@
text
@d320 3
@


1.13
log
@Remember to refresh fs_file pointer when we resize the buf
@
text
@d63 1
d238 1
@


1.12
log
@Convert to MIN/MAX, fix buffer resize
@
text
@d26 1
a26 1
file_grow(struct fs_file *fs, ulong newsize)
d62 1
d141 1
a141 1
			if (file_grow(fs, nsize)) {
d144 1
@


1.11
log
@Bad calculation for starting block number of buffer extent
@
text
@d43 1
a43 4
	incr = newsize - from;
	if (incr < EXTSIZ) {
		incr = EXTSIZ;
	}
d52 1
a52 4
		buflen = (a->a_len & (EXTSIZ-1)) + got;
		if (buflen > EXTSIZ) {
			buflen = EXTSIZ;
		}
d58 1
a58 1
				got, 0)) {
d176 1
a176 4
	len = a->a_len - extstart;
	if (len > EXTSIZ) {
		len = EXTSIZ;
	}
d249 1
a249 3
		if (step > cnt) {
			step = cnt;
		}
d348 1
a348 4
	len = m->m_arg;
	if (len > 256) {
		len = 256;
	}
@


1.10
log
@Fix access check, also fix offset calculations in bmap()
@
text
@d63 2
a64 1
		if (resize_buf(newstart & ~(EXTSIZ-1), buflen, 0)) {
@


1.9
log
@Fix loop advance for case of deleted entry
@
text
@d137 1
a137 1
	if ((pos + cnt) > fs->fs_len) {
d176 2
a177 2
	 * extoff holds the absolute sector for the start of the
	 *  current extent.
d179 1
a179 1
	extstart = ((nsize - extoff) & ~(EXTSIZ-1));
d471 1
a471 1
	 * Directory--only one is the root
d473 2
a474 2
	if (fs->fs_type == FT_DIR) {
		vfs_readdir(m, f);
d479 1
a479 1
	 * Access?
d481 2
a482 2
	if (!(f->f_perm & ACC_READ)) {
		msg_err(m->m_sender, EPERM);
@


1.8
log
@Rename to resize_buf
@
text
@d411 8
a418 3
		if (d->fs_name[0] & 0x80) {
			continue;
		}
d420 6
a425 6
		/*
		 * Check that it'll fit.  Leave loop when it doesn't.
		 */
		slen = strlen(d->fs_name)+1;
		if ((bufcnt + slen) >= len) {
			break;
a426 7

		/*
		 * Add name and update counters
		 */
		strcat(buf + bufcnt, d->fs_name);
		strcat(buf + bufcnt, "\n");
		bufcnt += slen;
@


1.7
log
@Fix incorrect alignment check.  Skip deleted files
in dir listing.
@
text
@d63 1
a63 1
		if (extend_buf(newstart & ~(EXTSIZ-1), buflen, 0)) {
@


1.6
log
@Fiddle bmap(), fix bad math for updating file length
@
text
@a354 9
	 * If the current seek position doesn't match a directory
	 * entry boundary, blow them out of the water.
	 */
	if ((f->f_pos % sizeof(struct fs_dirent)) != 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
d394 3
d403 10
@


1.5
log
@Fix calculation for extent offsets
@
text
@d125 2
a126 1
bmap(struct fs_file *fs, ulong pos, uint cnt, char **blkp, uint *stepp)
d137 1
a137 1
	if (pos > fs->fs_len) {
d151 2
d242 1
a242 1
		b2 = bmap(fs, pos, cnt, &blkp, &step);
d403 1
a403 1
			if (!bmap(fs, f->f_pos, len-bufcnt, &p, &step)) {
d518 1
a518 1
		b2 = bufs[nseg] = bmap(fs, f->f_pos, lim-cnt, &p, &sz);
@


1.4
log
@Clean up -Wall warnings
@
text
@d159 1
d161 2
a162 1
		if ((nsize >= a->a_start) && (nsize < (a->a_start+a->a_len))) {
d172 3
d176 1
a176 1
	extstart = (extoff & ~(EXTSIZ-1));
@


1.3
log
@Convert accesses to fs_file to use the routine getfs()
@
text
@d414 2
a415 1
		sprintf(buf + bufcnt, "%s\n", d->fs_name);
@


1.2
log
@Convert buffer resize to use only block addresses.  Do rest of
read/write support.
@
text
@d124 1
a124 1
static struct buf *
d144 1
a144 1
			if (file_setsize(fs, nsize)) {
d217 4
a220 1
	b = find_buf(o->o_file, o->o_len);
a221 1
	fs = index_buf(b, 0, 1);
d372 5
a376 1
	b = find_buf(f->f_file->o_file, f->f_file->o_len);
a377 1
	fs = index_buf(b, 0, 1);
d453 5
a457 2
	b = find_buf(f->f_file->o_file, f->f_file->o_len);
	fs = index_buf(b, 0, 1);
@


1.1
log
@Initial revision
@
text
@d30 1
a30 1
	daddr_t from;
d47 2
a48 1
	got = take_block(a->a_start + a->a_len, incr);
d50 21
d171 1
a171 1
	extstart = (extoff/EXTSIZ)*EXTSIZ;
d185 2
a186 2
	len = len - (extoff % EXTSIZ);
	*blkp = index_buf(b, extoff % EXTSIZ, len);
d232 1
a232 1
		b2 = bmap(fs, pos+OFF_DATA, cnt, &blkp, &step);
@
