head	1.32;
access;
symbols
	V1_3_1:1.31
	V1_3:1.31
	V1_2:1.30
	V1_1:1.27;
locks; strict;
comment	@ * @;


1.32
date	94.11.30.23.25.46;	author vandys;	state Exp;
branches;
next	1.31;

1.31
date	94.04.06.21.57.21;	author vandys;	state Exp;
branches;
next	1.30;

1.30
date	93.12.23.21.02.09;	author vandys;	state Exp;
branches;
next	1.29;

1.29
date	93.11.20.00.55.07;	author vandys;	state Exp;
branches;
next	1.28;

1.28
date	93.11.19.04.09.38;	author vandys;	state Exp;
branches;
next	1.27;

1.27
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.26;

1.26
date	93.10.23.21.15.57;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	93.10.23.20.19.57;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	93.10.18.21.53.42;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	93.10.18.20.43.53;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	93.10.16.00.10.58;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	93.10.15.22.29.10;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	93.10.06.21.44.42;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	93.10.06.18.41.26;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.09.28.00.31.56;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.09.27.23.09.45;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.09.18.18.10.15;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.09.14.23.37.42;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.09.12.23.36.33;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.09.11.19.06.08;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.08.31.03.06.37;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.08.31.00.38.09;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.08.30.22.33.08;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.08.30.21.37.07;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.08.30.03.39.18;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.08.29.22.26.48;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.29.19.12.00;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.29.10.20.03;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.10.00.19;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.28.14.16.08;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.27.16.46.57;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.27.15.40.46;	author vandys;	state Exp;
branches;
next	;


desc
@File name lookup
@


1.32
log
@Fix ref handling during file rename
@
text
@/*
 * open.c
 *	Routines for opening, closing, creating  and deleting files
 */
#include "vstafs.h"
#include "alloc.h"
#include "buf.h"
#include <std.h>
#include <sys/param.h>
#include <sys/assert.h>
#include <hash.h>

static struct hash *rename_pending;

/*
 * partial_trunc()
 *	Trim back the block allocation on an allocation unit
 */
static void
partial_trunc(struct alloc *a, uint newsize)
{
	uint topbase;

	ASSERT_DEBUG(newsize > 0, "partial_trunc: 0 size");
	ASSERT_DEBUG(newsize <= a->a_len, "partial_trunc: grow");

	/*
	 * If not even a single block has been freed, the partial
	 * trunc comes to a no-op from the perspective of block
	 * allocation.
	 */
	if (newsize == a->a_len) {
		return;
	}

	/*
	 * Inval buffer extents beyond last one with data
	 */
	topbase = roundup(newsize, EXTSIZ);
	if (a->a_len > topbase) {
		inval_buf(a->a_start + topbase, a->a_len - topbase);
	}

	/*
	 * Resize extent, freeing trailing data
	 */
	free_block(a->a_start + newsize, a->a_len - newsize);
	a->a_len = newsize;
	topbase = (newsize & ~(EXTSIZ-1));
	resize_buf(a->a_start + topbase, newsize - topbase, 0);
}

/*
 * file_shrink()
 *	Trim a file down to the specified length
 *
 * Does not handle freeing of actual fs_file data.
 */
static void
file_shrink(struct openfile *o, ulong len)
{
	struct fs_file *fs;
	struct buf *b;
	ulong pos;
	uint idx, y;
	struct alloc *a;

	ASSERT_DEBUG(len >= sizeof(struct fs_file), "file_shrink: too small");

	/*
	 * Get file, apply sanity checks
	 */
	fs = getfs(o, &b);
	ASSERT(fs, "file_shrink: can't map file");
	ASSERT_DEBUG(len <= fs->fs_len, "file_shrink: grow");

	/*
	 * Scan the extents
	 */
	pos = 0;
	a = fs->fs_blks;
	for (idx = 0; idx < fs->fs_nblk; ++idx,++a) {
		ulong npos;

		/*
		 * Continue loop while offset is below
		 */
		npos = pos + stob(a->a_len);
		if (npos <= len) {
			pos = npos;
			continue;
		}

		/*
		 * Trim extent if it's partially truncated
		 */
		y = btors(len - pos);
		if (y > 0) {
			/*
			 * Shave it
			 */
			partial_trunc(a, y);

			/*
			 * If this was extent 0, our buffer data
			 * might move on the resize.  Re-access
			 * it.
			 */
			if (idx == 0) {
				fs = index_buf(b, 0, 1);
				a = fs->fs_blks;
			}

			/*
			 * This extent is finished, advance to next
			 */
			a += 1;
			idx += 1;
		}

		/*
		 * Now dump the remaining extents
		 */
		for (y = idx; y < fs->fs_nblk; ++y,++a) {
			inval_buf(a->a_start, a->a_len);
			free_block(a->a_start, a->a_len);
		}
		fs->fs_nblk = idx;
		break;
	}

	/*
	 * Flag buffer as dirty, update length
	 */
	dirty_buf(b);
	fs->fs_len = len;
	o->o_len = btors(len);
	if (o->o_hiwrite > len) {
		o->o_hiwrite = len;
	}
	sync_buf(b);
}

/*
 * getfs()
 *	Given openfile, return pointer to its fs_file
 *
 * The returned value is in the buffer pool, and can only be
 * used until the next request is made against the buffer pool.
 *
 * If bp is non-zero, the associated buffer header is filled
 * into this pointer.
 */
struct fs_file *
getfs(struct openfile *o, struct buf **bp)
{
	struct fs_file *fs;
	struct buf *b;

	b = find_buf(o->o_file, MIN(o->o_len, EXTSIZ));
	if (!b) {
		return(0);
	}
	fs = index_buf(b, 0, 1);
	if (bp) {
		*bp = b;
	}
	return(fs);
}

/*
 * findent()
 *	Given a buffer-full of fs_dirent's, look up a filename
 *
 * Returns a pointer to a dirent on success, 0 on failure.
 */
static struct fs_dirent *
findent(struct fs_dirent *d, uint nent, char *name)
{
	for ( ; nent > 0; ++d,--nent) {
		/*
		 * No more entries in file
		 */
		if (d->fs_clstart == 0) {
			return(0);
		}

		/*
		 * Deleted file
		 */
		if (d->fs_name[0] & 0x80) {
			continue;
		}

		/*
		 * Keep going while no match
		 */
		if (strcmp(d->fs_name, name)) {
			continue;
		}

		/*
		 * Return matching dir entry
		 */
		return(d);
	}
	return(0);
}

/*
 * dir_lookup()
 *	Given open dir, look for name
 *
 * On success, returns the openfile; on failure, 0.
 *
 * "b" is assumed locked on entry; will remain locked in this routine.
 */
static struct openfile *
dir_lookup(struct buf *b, struct fs_file *fs, char *name,
	struct fs_dirent **dep, struct buf **bp)
{
	struct buf *b2;
	uint extent;
	ulong left = fs->fs_len;

	/*
	 * Walk the directory entries one extent at a time
	 */
	for (extent = 0; extent < fs->fs_nblk; ++extent) {
		uint x, len;
		struct alloc *a = &fs->fs_blks[extent];

		/*
		 * Walk through an extent one buffer-full at a time
		 */
		for (x = 0; x < a->a_len; x += EXTSIZ) {
			struct fs_dirent *d;

			ASSERT_DEBUG(left != 0, "dir_lookup: left == 0");
			/*
			 * Figure out size of next buffer-full
			 */
			len = a->a_len - x;
			if (len > EXTSIZ) {
				len = EXTSIZ;
			}

			/*
			 * Map it in
			 */
			b2 = find_buf(a->a_start+x, len);
			if (b2 == 0) {
				return(0);
			}
			d = index_buf(b2, 0, len);
			len = stob(len);

			/*
			 * Ignore unused trailing part of last extent
			 */
			len = MIN(len, left);
			left -= len;

			/*
			 * Special case for initial data in file
			 */
			if ((extent == 0) && (x == 0)) {
				d = (struct fs_dirent *)
					((char *)d + sizeof(struct fs_file));
				len -= sizeof(struct fs_file);
			}

			/*
			 * Look for our filename
			 */
			d = findent(d, len/sizeof(struct fs_dirent), name);
			if (d) {
				struct openfile *o;

				/*
				 * Found it.  Get node, and fill in
				 * our information.
				 */
				o = get_node(d->fs_clstart);
				if (!o) {
					return(0);
				}
				if (dep) {
					*dep = d;
					*bp = b2;
				}
				return(o);
			}
		}
	}
	return(0);
}

/*
 * findfree()
 *	Find the next open directory slot
 */
static struct fs_dirent *
findfree(struct fs_dirent *d, uint nent)
{
	while (nent > 0) {
		if ((d->fs_clstart == 0) || (d->fs_name[0] & 0x80)) {
			return(d);
		}
		nent -= 1;
		d += 1;
	}
	return(0);
}

/*
 * create_file()
 *	Create the initial "file" contents
 */
static struct openfile *
create_file(struct file *f, uint type)
{
	daddr_t da;
	struct buf *b;
	struct fs_file *d;
	struct prot *p;
	struct openfile *o;

	/*
	 * Get the block, map it
	 */
	da = alloc_block(1);
	if (da == 0) {
		return(0);
	}
	b = find_buf(da, 1);
	if (b == 0) {
		free_block(da, 1);
		return(0);
	}
	d = index_buf(b, 0, 1);

	/*
	 * Fill in the fields
	 */
	d->fs_prev = 0;
	d->fs_rev = 1;
	d->fs_len = sizeof(struct fs_file);
	d->fs_type = type;
	d->fs_nlink = 1;
	d->fs_nblk = 1;
	d->fs_blks[0].a_start = da;
	d->fs_blks[0].a_len = 1;

	/*
	 * Default protection, use 0'th perm
	 */
	p = &d->fs_prot;
	bzero(p, sizeof(*p));
	p->prot_len = PERM_LEN(&f->f_perms[0]);
	bcopy(f->f_perms[0].perm_id, p->prot_id, PERMLEN);
	p->prot_bits[p->prot_len-1] =
		ACC_READ|ACC_WRITE|ACC_CHMOD;
	d->fs_owner = f->f_perms[0].perm_uid;

	/*
	 * Allocate an openfile to it
	 */
	o = get_node(da);
	if (o == 0) {
		free_block(da, 1);
	}

	/*
	 * Flush out info to disk, and return openfile
	 */
	dirty_buf(b);
	sync_buf(b);
	return(o);
}

/*
 * uncreate_file()
 *	Release resources of an openfile
 */
static void
uncreate_file(struct openfile *o)
{
	struct fs_file *fs;
	struct alloc *a;

	ASSERT(o->o_refs == 1, "uncreate_file: refs");

	/*
	 * Access file structure info
	 */
	fs = getfs(o, 0);
	ASSERT(fs, "uncreate_file: buffer access failed");

	/*
	 * Dump all but the fs_file.  The buffer can move; re-map
	 * it.
	 */
	file_shrink(o, sizeof(struct fs_file));
	fs = getfs(o, 0);

	/*
	 * Dump the file header, and free the openfile
	 */
	a = &fs->fs_blks[0];
	ASSERT_DEBUG(a->a_len == 1, "uncreate_file: too many left");
	free_block(a->a_start, 1);
	inval_buf(a->a_start, 1);
	deref_node(o);
}

/*
 * dir_newfile()
 *	Create a new entry in the current directory
 *
 * On success new openfile will have one reference.
 */
static struct openfile *
dir_newfile(struct file *f, char *name, int type)
{
	struct buf *b, *b2;
	struct fs_file *fs;
	uint extent;
	ulong off;
	struct openfile *o;
	struct fs_dirent *d;
	int err = 0;

	/*
	 * Access file structure of enclosing dir
	 */
	fs = getfs(f->f_file, &b);
	if (fs == 0) {
		return(0);
	}
	lock_buf(b);

	/*
	 * Get the openfile first
	 */
	o = create_file(f, type);
	if (o == 0) {
		return(0);
	}

	/*
	 * Walk the directory entries one extent at a time
	 */
	off = sizeof(struct fs_file);
	for (extent = 0; extent < fs->fs_nblk; ++extent) {
		uint x;
		ulong len;
		struct alloc *a = &fs->fs_blks[extent];

		/*
		 * Walk through an extent one buffer-full at a time
		 */
		for (x = 0; x < a->a_len; x += EXTSIZ) {
			struct fs_dirent *dstart;

			/*
			 * Figure out size of next buffer-full
			 */
			len = MIN(a->a_len - x, EXTSIZ);

			/*
			 * Map it in
			 */
			b2 = find_buf(a->a_start+x, len);
			if (b2 == 0) {
				err = 1;
				goto out;
			}
			d = index_buf(b2, 0, len);
			len = stob(len);

			/*
			 * Special case for initial data in file
			 */
			if ((extent == 0) && (x == 0)) {
				d = (struct fs_dirent *)((char *)d + off);
				len -= sizeof(struct fs_file);
			}

			/*
			 * Look for a free slot
			 */
			dstart = d;
			len = MIN(len, fs->fs_len - off);
			d = findfree(dstart, len/sizeof(struct fs_dirent));
			if (d) {
				off += ((char *)d - (char *)dstart);
				goto out;
			}
			off += len;
		}
	}

	/*
	 * No luck with existing blocks.  Use bmap() to map in some
	 * more storage.
	 */
	ASSERT_DEBUG(off == fs->fs_len, "dir_newfile: off/len skew");
	{
		uint dummy;

		b2 = bmap(b, fs, fs->fs_len, sizeof(struct fs_dirent),
			(char **)&d, &dummy);
		if (b2 == b) {
			fs = index_buf(b, 0, 1);
		}
	}
	if (b2 == 0) {
		err = 1;
	}
out:
	/*
	 * On error, release germinal file allocation, return 0
	 */
	if (err) {
		uncreate_file(o);
		return(0);
	}

	/*
	 * We have a slot, so fill it in & return success
	 */
	strcpy(d->fs_name, name);
	d->fs_clstart = o->o_file;
	if (off > f->f_file->o_hiwrite) {
		f->f_file->o_hiwrite = off;
	}

	/*
	 * Update dir file's length
	 */
	off += sizeof(struct fs_dirent);
	if (off > fs->fs_len) {
		fs->fs_len = off;
		dirty_buf(b);
	} else {
		off = MAX(off, f->f_file->o_hiwrite);
		if (fs->fs_len > off) {
			file_shrink(f->f_file, off);
		}
	}

	dirty_buf(b2);
	sync_buf(b2);
	sync_buf(b);
	unlock_buf(b);
	return(o);
}

/*
 * vfs_open()
 *	Main entry for processing an open message
 */
void
vfs_open(struct msg *m, struct file *f)
{
	struct buf *b;
	struct openfile *o;
	struct fs_file *fs;
	uint x, want;

	/*
	 * Get file header, but don't wire down
	 */
	fs = getfs(f->f_file, &b);
	if (!fs) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Have to be in dir to open down into a file
	 */
	if (fs->fs_type != FT_DIR) {
		msg_err(m->m_sender, ENOTDIR);
		return;
	}

	/*
	 * Look up name, make sure "fs" stays valid
	 */
	lock_buf(b);
	o = dir_lookup(b, fs, m->m_buf, 0, 0);
	unlock_buf(b);

	/*
	 * No such file--do they want to create?
	 */
	if (!o && !(m->m_arg & ACC_CREATE)) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * If it's a new file, allocate the entry now.
	 */
	if (!o) {
		/*
		 * Allowed?
		 */
		if ((f->f_perm & (ACC_WRITE|ACC_CHMOD)) == 0) {
			msg_err(m->m_sender, EPERM);
			return;
		}

		/*
		 * Failure?
		 */
		o = dir_newfile(f, m->m_buf, (m->m_arg & ACC_DIR) ?
				FT_DIR : FT_FILE);
		if (o == 0) {
			msg_err(m->m_sender, ENOMEM);
			return;
		}

		/*
		 * Move to new node
		 */
		deref_node(f->f_file);
		f->f_file = o;
		f->f_perm = ACC_READ|ACC_WRITE|ACC_CHMOD;
		m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Check permission
	 */
	fs = getfs(o, &b);
	want = m->m_arg & (ACC_READ|ACC_WRITE|ACC_CHMOD);
	x = perm_calc(f->f_perms, f->f_nperm, &fs->fs_prot);
	if ((want & x) != want) {
		deref_node(o);
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * If they wanted it truncated, do it now
	 */
	if (m->m_arg & ACC_CREATE) {
		if ((x & ACC_WRITE) == 0) {
			deref_node(o);
			msg_err(m->m_sender, EPERM);
			return;
		}
		file_shrink(o, sizeof(struct fs_file));
	}

	/*
	 * Move to this file
	 */
	deref_node(f->f_file);
	f->f_file = o;
	f->f_perm = m->m_arg | (x & ACC_CHMOD);
	m->m_nseg = m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * vfs_close()
 *	Do closing actions on a file
 */
void
vfs_close(struct file *f)
{
	struct openfile *o;

	o = f->f_file;
	ASSERT_DEBUG(o, "vfs_close: no openfile");
	if (o->o_refs == 1) {
		/*
		 * Files are extended with a length which reflects
		 * the extent pre-allocation.  On final close, we
		 * trim this pre-allocated space back, and update
		 * the file's length to indicate just the true
		 * data.
		 */
		if (f->f_perm & ACC_WRITE) {
			file_shrink(o, o->o_hiwrite);
		}
		sync();
	}
	deref_node(o);
}

/*
 * do_unhash()
 *	Function to do the unhash() call from a child thread
 */
static ulong unhash_fid;
static void
do_unhash(void)
{
	extern port_t rootport;
	extern void unhash();

	unhash(rootport, unhash_fid);
	_exit(0);
}

/*
 * vfs_remove()
 *	Remove an entry in the current directory
 */
void
vfs_remove(struct msg *m, struct file *f)
{
	struct buf *b, *b2;
	struct fs_file *fs;
	struct openfile *o;
	uint x;
	struct fs_dirent *de;

	/*
	 * Look at file structure
	 */
	fs = getfs(f->f_file, &b);
	if (fs == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Have to be in a dir
	 */
	if (fs->fs_type != FT_DIR) {
		msg_err(m->m_sender, ENOTDIR);
		return;
	}

	/*
	 * Look up entry.  Bail if no such file.
	 */
	lock_buf(b);
	o = dir_lookup(b, fs, m->m_buf, &de, &b2);
	unlock_buf(b);
	if (o == 0) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * Check permission
	 */
	x = perm_calc(f->f_perms, f->f_nperm, &fs->fs_prot);
	if ((x & (ACC_WRITE|ACC_CHMOD)) == 0) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Try unhashing if it might be the only other reference
	 */
	if (o->o_refs == 2) {
		/*
		 * Since a closing portref needs to handshake
		 * with the server, use a child thread to do
		 * the dirty work.
		 */
		unhash_fid = o->o_file;
		(void)tfork(do_unhash);

		/*
		 * Release our ref and tell the requestor he
		 * might want to try again.
		 */
		msg_err(m->m_sender, EAGAIN);
		return;
	}

	/*
	 * Can't be any other users
	 */
	if (o->o_refs > 1) {
		msg_err(m->m_sender, EBUSY);
		return;
	}

	/*
	 * Zap the dir entry, then blocks
	 */
	de->fs_name[0] |= 0x80; dirty_buf(b2); sync_buf(b2);
	uncreate_file(o);

	/*
	 * Return success
	 */
	m->m_buflen = m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * get_dirent()
 *	Look up named entry, return pointer to it
 *
 * On error, returns 0 and puts a string in err.  On success, returns
 * dir entry pointer and buf for it in bp.  Buffer has been locked.
 */
static struct fs_dirent *
get_dirent(struct file *f, char *name, struct buf **bpp, char **errp,
	int create, struct openfile **op)
{
	struct fs_file *fs;
	struct buf *b;
	struct fs_dirent *de;
	struct openfile *o;

	/*
	 * Get file header, make sure it's a directory
	 */
	fs = getfs(f->f_file, &b);
	if (fs == 0) {
		*errp = strerror();
		return(0);
	}

	/*
	 * Have to be in a dir
	 */
	if (fs->fs_type != FT_DIR) {
		*errp = ENOTDIR;
		return(0);
	}

	/*
	 * Look up entry
	 */
	lock_buf(b);
	o = dir_lookup(b, fs, name, &de, bpp);
	unlock_buf(b);

	/*
	 * If not there, see about creating
	 */
	if ((o == 0) && create) {
		/*
		 * Wire down parent buf, create new file
		 */
		lock_buf(b);
		o = dir_newfile(f, name, FT_FILE);
		if (o == 0) {
			unlock_buf(b);
			*errp = EINVAL;
			return(0);
		}

		/*
		 * Get new dir entry, drop ref from dir_newfile()
		 * now that dir_lookup() has taken one.
		 */
		o = dir_lookup(b, fs, name, &de, bpp);
		deref_node(o);
		unlock_buf(b);
		ASSERT(o, "get_dirent: can't find created file");
	}

	/*
	 * Free any storage, bomb on the destination being a directory
	 */
	if (create) {
		fs = getfs(o, &b);
		if (fs->fs_type == FT_DIR) {
			*errp = EISDIR;
			return(0);
		}
		file_shrink(o, sizeof(struct fs_file));
	}

	/*
	 * Give open file back, or release our hold on it
	 */
	if (op) {
		*op = o;
	} else {
		deref_node(o);
	}
	ASSERT_DEBUG(de, "get_dirent: !de");
	lock_buf(*bpp);
	return(de);
}

/*
 * do_rename()
 *	Given open directories and filenames, rename an entry
 *
 * Returns an error string or 0 for success.
 */
static char *
do_rename(struct file *fsrc, char *src, struct file *fdest, char *dest)
{
	struct fs_dirent *desrc, *dedest;
	struct buf *bsrc = 0, *bdest = 0;
	struct openfile *odest;
	char *err;
	daddr_t blktmp;

	/*
	 * Get pointers to source and destination directory
	 * entries.
	 */
	desrc = get_dirent(fsrc, src, &bsrc, &err, 0, 0);
	if (desrc == 0) {
		return(err);
	}
	dedest = get_dirent(fdest, dest, &bdest, &err, 1, &odest);
	if (dedest == 0) {
		unlock_buf(bsrc);
		return(err);
	}

	/*
	 * Swap who holds which file
	 */
	blktmp = desrc->fs_clstart;
	desrc->fs_clstart = dedest->fs_clstart;
	dedest->fs_clstart = blktmp;

	/*
	 * Mark the two directory blocks dirty, and release their locks
	 */
	dirty_buf(bsrc); unlock_buf(bsrc);
	dirty_buf(bdest); unlock_buf(bdest);

	/*
	 * Delete the old one now
	 */
	ASSERT_DEBUG(odest->o_refs == 1, "do_rename: o_refs != 1");
	desrc->fs_name[0] |= 0x80;
	uncreate_file(odest);

	/*
	 * Success
	 */
	sync();
	return(0);
}

/*
 * dos_rename()
 *	Rename one dir entry to another
 *
 * For move of directory, just protect against cycles and update
 * the ".." entry in the directory after the move.
 */
void
vfs_rename(struct msg *m, struct file *f)
{
	struct file *f2;
	char *errstr;
	extern int valid_fname(char *, int);

	/*
	 * Sanity
	 */
	if ((m->m_arg1 == 0) || !valid_fname(m->m_buf, m->m_buflen)) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * On first use, create the rename-pending hash
	 */
	if (rename_pending == 0) {
		rename_pending = hash_alloc(16);
		if (rename_pending == 0) {
			msg_err(m->m_sender, strerror());
			return;
		}
	}

	/*
	 * Phase 1--register the source of the rename
	 */
	if (m->m_arg == 0) {
		/*
		 * Transaction ID collision?
		 */
		if (hash_lookup(rename_pending, m->m_arg1)) {
			msg_err(m->m_sender, EBUSY);
			return;
		}

		/*
		 * Insert in hash
		 */
		if (hash_insert(rename_pending, m->m_arg1, f)) {
			msg_err(m->m_sender, strerror());
			return;
		}

		/*
		 * Flag open file as being involved in this
		 * pending operation.
		 */
		f->f_rename_id = m->m_arg1;
		f->f_rename_msg = *m;
		return;
	}

	/*
	 * Otherwise it's the completion
	 */
	f2 = hash_lookup(rename_pending, m->m_arg1);
	if (f2 == 0) {
		msg_err(m->m_sender, ESRCH);
		return;
	}
	(void)hash_delete(rename_pending, m->m_arg1);

	/*
	 * Do our magic
	 */
	errstr = do_rename(f2, f2->f_rename_msg.m_buf, f, m->m_buf);
	if (errstr) {
		msg_err(m->m_sender, errstr);
		msg_err(f2->f_rename_msg.m_sender, errstr);
	} else {
		m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		msg_reply(f2->f_rename_msg.m_sender, m);
	}

	/*
	 * Clear state
	 */
	f2->f_rename_id = 0;
}

/*
 * cancel_rename()
 *	A client exit()'ed before completing a rename.  Clean up.
 */
void
cancel_rename(struct file *f)
{
	(void)hash_delete(rename_pending, f->f_rename_id);
	f->f_rename_id = 0;
}
@


1.31
log
@Add rename() support, pass 1
@
text
@d420 2
d848 3
d858 5
d864 1
d889 2
@


1.30
log
@Buffer could move; re-access
@
text
@d11 1
d13 2
d800 237
@


1.29
log
@Add unhashing of a.out's
@
text
@d398 2
a399 1
	 * Dump all but the fs_file
d402 1
@


1.28
log
@Fix missing cap to length of file
@
text
@d692 15
d753 20
@


1.27
log
@Source reorg
@
text
@d479 1
a479 2
				d = (struct fs_dirent *)
					((char *)d + sizeof(struct fs_file));
d484 1
a484 1
			 * Look for our filename
d487 1
@


1.26
log
@Fix bug where o_len was getting the wrong units.  Also fix
bug where file truncation was not reflected in o_hiwrite.
@
text
@d5 3
a7 3
#include <vstafs/vstafs.h>
#include <vstafs/alloc.h>
#include <vstafs/buf.h>
@


1.25
log
@Shrinking a file can shrink the buffer and realloc() it--which can
move the buffer location.  Fill in the dir entry first so it
doesn't matter.
@
text
@d133 5
a137 1
	o->o_len = fs->fs_len = len;
@


1.24
log
@Check the right file node for permission
@
text
@d520 9
a541 8
	/*
	 * We have a slot, so fill it in & return success
	 */
	strcpy(d->fs_name, name);
	d->fs_clstart = o->o_file;
	if (off > f->f_file->o_hiwrite) {
		f->f_file->o_hiwrite = off;
	}
d544 1
a544 3
	if (b != b2) {
		sync_buf(b);
	}
@


1.23
log
@Use MIN macro, only skip file header for first part of first extent
@
text
@d630 1
d643 5
@


1.22
log
@Keep o_hiwrite up to date for dirs, use this to correctly
trim back excess dir growth without trimming back true dir
entries when we reuse an open slot in the middle of a file.
@
text
@d458 1
a458 4
			len = a->a_len - x;
			if (len > EXTSIZ) {
				len = EXTSIZ;
			}
d474 1
a474 1
			if (x == 0) {
@


1.21
log
@Add an extra assertion, for sanity
@
text
@d436 3
d529 5
a533 2
	} else if (fs->fs_len > off) {
		file_shrink(f->f_file, off);
d541 3
@


1.20
log
@Get length & file length limit right in directory scan
@
text
@d22 1
@


1.19
log
@Handle directory grow better--trim excess from expansion
@
text
@d216 1
d231 1
d248 1
d251 6
d259 1
a259 1
			if (x == 0) {
@


1.18
log
@Only verify "meaningful" protection bits
@
text
@d432 2
a433 1
		uint x, len;
d459 1
d473 2
a474 2
			dstart = d =
				findfree(d, len/sizeof(struct fs_dirent));
a479 1

d487 1
d493 3
a498 1
		goto out;
a499 3
	ASSERT_DEBUG(off >= sizeof(struct fs_dirent),
		"dir_newfile: bad growth");

d516 2
@


1.17
log
@Implement o_hiwrite to trim off excess preallocation
@
text
@d541 1
a541 1
	uint x;
d611 1
d613 1
a613 1
	if ((m->m_arg & x) != m->m_arg) {
@


1.16
log
@Cap length at EXTSIZ for struct fs_file length
@
text
@d41 1
a41 1
	 * Consider last buffer extent with data, and resize
d43 2
a46 2
	free_block(a->a_start + newsize, a->a_len - newsize);
	a->a_len = newsize;
d107 1
d647 10
@


1.15
log
@Fix up truncation when deleting or O_CREAT'ing a file
@
text
@d151 1
a151 1
	b = find_buf(o->o_file, o->o_len);
@


1.14
log
@Fix dangling node ref
@
text
@d9 1
d13 37
d96 1
a96 1
			 * Reconfigure any buffer, free excess storage.
d98 1
a98 7
			if (a->a_len > EXTSIZ) {
				inval_buf(a->a_start + EXTSIZ,
					a->a_len - EXTSIZ);
			}
			resize_buf(a->a_start, y, 0);
			free_block(a->a_start + y, a->a_len - y);
			a->a_len = y;
d105 3
a107 1
			fs = index_buf(b, 0, 1);
@


1.13
log
@Fiddle routines to get dir_lookup working, and useful for both
open and unlink.
@
text
@d593 1
@


1.12
log
@Use central ref routine
@
text
@d12 90
d132 1
a132 1
 * Returns a pointer to an openfile on success, 0 on failure.
d134 1
a134 1
static struct openfile *
d160 1
a160 2
		 * Found it!  Let our node layer take it, and return the
		 * result.
d162 1
a162 1
		return(get_node(d->fs_clstart));
d176 2
a177 1
dir_lookup(struct buf *b, struct fs_file *fs, char *name)
a193 1
			struct openfile *o;
d224 16
a239 2
			o = findent(d, len/sizeof(struct fs_dirent), name);
			if (o) {
d337 2
a338 2
	int x;
	struct fs_file *fs, fshold;
a346 1
	fshold = *fs;
d349 1
a349 3
	 * Free all allocated blocks, then free openfile itself.  Note we
	 * work our way from the top down, so we hit the buffer containing
	 * the file structure itself last.
d351 1
a351 3
	for (x = fshold.fs_nblk-1; x >= 0; --x) {
		struct alloc *a = &fshold.fs_blks[x];
		uint y;
d353 8
a360 19
		/*
		 * Invalidate out any buffers associated with
		 * the file's data.
		 */
		for (y = 0; y < a->a_len; y += EXTSIZ) {
			ulong len;

			len = a->a_len - y;
			if (len > EXTSIZ)
				len = EXTSIZ;
			inval_buf(a->a_start + y, (uint)y);
		}

		/*
		 * Now release the physical storage
		 */
		free_block(a->a_start, a->a_len);
	}
	free(o);
d529 1
a529 1
	o = dir_lookup(b, fs, m->m_buf);
d545 8
d578 1
d587 1
a587 9
		struct openfile *o2;

		/*
		 * Much easier to just allocate a new initial file, and
		 * slam it onto the existintg open file description.
		 */
		o2 = create_file(f, (m->m_arg & ACC_DIR) ? FT_DIR : FT_FILE);
		uncreate_file(o);
		*o = *o2;
d593 1
a593 1
	f->f_file = o; ref_node(o);
d623 1
a623 1
	struct buf *b;
d627 1
d632 1
a632 1
	fs = getfs(o, &b);
d650 1
a650 1
	o = dir_lookup(b, fs, m->m_buf);
d675 1
a675 1
	 * Zap the blocks
d677 1
@


1.11
log
@New arg to bmap()
@
text
@d505 1
a505 1
	f->f_file = o; o->o_refs += 1;
@


1.10
log
@Update directory file's length on creation of new element
@
text
@d367 1
a367 1
		b2 = bmap(fs, fs->fs_len, sizeof(struct fs_dirent),
@


1.9
log
@Files initially start with a size of sizeof(struct fs_file),
which occupies the intial bytes in the file allocation.
@
text
@d285 2
a286 1
	uint off, extent;
d308 1
d317 2
d349 2
a350 1
			d = findfree(d, len/sizeof(struct fs_dirent));
d352 1
d355 1
d364 6
a369 2
	b2 = bmap(fs, fs->fs_len, sizeof(struct fs_dirent),
		(char **)&d, &off);
d387 9
d402 4
d521 3
a523 2
	if (o) {
		deref_node(o);
d525 1
@


1.8
log
@Remove redundant field
@
text
@d193 1
a193 1
	d->fs_len = 0;
@


1.7
log
@Clean up -Wall warnings
@
text
@a190 1
	d->fs_clstart = da;
@


1.6
log
@Convert accesses to fs_file to use the routine getfs()
@
text
@a88 1
	ulong off;
d497 2
a498 1
	if (o = f->f_file) {
@


1.5
log
@First pass, file deletion/truncation
@
text
@d12 27
d237 1
a237 2
	struct buf *b;
	struct fs_file *fs;
d244 3
a246 3
	b = find_buf(o->o_file, o->o_len);
	ASSERT(b, "uncreate_file: lost buf");
	fs = index_buf(b, 0, 1);
d253 2
a254 2
	for (x = fs->fs_nblk-1; x >= 0; --x) {
		struct alloc *a = &fs->fs_blks[x];
d295 2
a296 2
	b = find_buf(f->f_file->o_file, f->f_file->o_len);
	if (b == 0) {
a299 1
	fs = index_buf(b, 0, 1);
d402 2
a403 3
	o = f->f_file;
	b = find_buf(o->o_file, o->o_len);
	if (!b || !(fs = index_buf(b, 0, 1))) {
d518 2
a519 2
	b = find_buf(o->o_file, o->o_len);
	if (b == 0) {
a522 1
	fs = index_buf(b, 0, 1);
@


1.4
log
@Fix up open() functionality
@
text
@d9 1
d209 1
a209 1
	uint x;
d219 1
d223 3
a225 1
	 * Free all allocated blocks, then free openfile itself
d227 1
a227 1
	for (x = 0; x < fs->fs_nblk; ++x) {
d445 9
a453 1
		blk_trunc(o);
d492 10
d504 1
a504 1
	if (f->f_file) {
d512 3
a514 1
	o = dir_lookup(m->m_buf);
d523 1
a523 1
	x = perm_calc(f->f_perms, f->f_nperm, &o->o_prot);
@


1.3
log
@Further file creation/deletion stuff
@
text
@d7 1
d77 1
d118 17
d137 1
a137 2
 *
 *
d143 1
a143 1
	struct fs_dirent *d;
d145 1
d331 1
a331 1
		&d, &off);
d351 2
a352 2
	strcpy(d->d_name, name);
	d->d_clstart = o->o_file;
d354 1
a354 1
	sync_buf(b2)
d368 1
d392 1
a392 1
	o = dir_lookup(b, fs, m->m_buf, findent);
d431 1
a431 1
	x = perm_calc(f->f_perms, f->f_nperm, &o->o_prot);
d474 2
d480 1
a480 1
	 * Have to be in root dir
d508 1
a508 1
	if (o->o_refs > 0) {
d516 1
a516 6
	blk_trunc(o);

	/*
	 * Free the node memory
	 */
	freeup(o);
@


1.2
log
@First pass, file creation
@
text
@d121 1
a121 1
create_file(uint type)
d126 1
a150 1
	d->fs_prot = XXX
d156 11
d183 47
a231 2
 *
 * "b" is locked throughout.
d234 1
a234 1
dir_newfile(struct buf *b, struct fs_file *fs, char *name, int type)
d236 3
a238 3
	ulong off;
	struct buf *b2;
	uint extent;
d240 12
d256 1
a256 2

	o = create_file(type);
a268 2
			struct fs_dirent *d;

d282 2
a283 1
				return(0);
d301 1
a301 10
				/*
				 * Found a slot.  Put our data
				 * into place, make sure it's registered
				 * on-disk, and return an openfile.
				 */
				strcpy(d->d_name, name);
				d->d_clstart = o->o_file;
				dirty_buf(b2);
				sync_buf(b2)
				return(o);
d311 8
a318 11
	{
		struct fs_dirent *dp;
		uint size;

		b2 = bmap(fs, fs->fs_len, sizeof(struct fs_dirent),
			&dp, &size);
		if (b2 == 0) {
			return(0);
		}
		ASSERT_DEBUG(size >= sizeof(struct fs_dirent),
			"dir_newfile: bad growth");
d320 7
a326 8
		/*
		 * Now we have a slot, so fill it in & return success
		 */
		strcpy(dp->d_name, name);
		dp->d_clstart = o->o_file;
		dirty_buf(b2);
		sync_buf(b2)
		return(o);
d330 7
a336 2
	 * We have failed.
	return(0);
d390 1
a390 1
		o = dir_newfile(b, fs, m->m_buf, (m->m_arg & ACC_DIR) ?
@


1.1
log
@Initial revision
@
text
@d6 1
d20 14
d95 9
d116 56
d174 2
d180 99
d314 1
a314 1
	o = dir_lookup(b, fs, m->m_buf);
@
