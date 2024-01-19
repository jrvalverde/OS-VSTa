head	1.6;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.6
date	94.04.11.00.35.24;	author vandys;	state Exp;
branches;
next	1.5;

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
date	93.06.30.19.52.24;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.39.21;	author vandys;	state Exp;
branches;
next	;


desc
@File open/create
@


1.6
log
@Fix warnings
@
text
@/*
 * Filename:	open.c
 * Developed:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Originated:	Andy Valencia
 * Last Update: 8th April 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description: Routines for opening, closing, creating  and deleting files
 *
 * Note that we don't allow subdirectories for bfs, which simplifies
 * things.
 */


#include <hash.h>
#include <std.h>
#include <stdio.h>
#include <sys/assert.h>
#include "bfs.h"


extern struct super *sblock;
static int nwriters = 0;	/* Number of writers active */
static struct hash *rename_pending = NULL;
				/* Tabulate pending renames */


/*
 * move_file()
 *	Transfer the current file of a struct file to the given inode
 */
static void
move_file(struct file *f, struct inode *i, int writing)
{
	ASSERT_DEBUG(f->f_inode->i_num == ROOTINODE, "move_file: not root");
	f->f_inode = i;
	f->f_pos = 0L;
	if (f->f_write == writing)
		nwriters += 1;
}


/*
 * bfs_open()
 *	Main entry for processing an open message
 */
void
bfs_open(struct msg *m, struct file *f)
{
	struct inode *i;
	int iexists = 0;

	/*
	 * Have to be in root dir to open down into a file
	 */
	if (f->f_inode->i_num != ROOTINODE) {
		msg_err(m->m_sender, ENOTDIR);
		return;
	}

	/*
	 * Check for permission
	 */
	if (m->m_arg & (ACC_WRITE|ACC_CREATE|ACC_DIR)) {
		/*
		 * No subdirs in a boot filesystem
		 */
		if (m->m_arg & ACC_DIR) {
			msg_err(m->m_sender, EINVAL);
			return;
		}

		/*
		 * Only one writer at a time
		 */
		if (nwriters > 0) {
			msg_err(m->m_sender, EBUSY);
			return;
		}

		/*
		 * Insufficient priveleges
		 */
		if (f->f_write == 0) {
			msg_err(m->m_sender, EPERM);
			return;
		}
	}

	/*
	 * Look up name
	 */
	i = ino_lookup(m->m_buf);
	if (i) {
		iexists = 1;
	}

	/*
	 * No such file--do they want to create?
	 */
	if (!iexists && !(m->m_arg & ACC_CREATE)) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * If it's a new file, allocate the entry now.
	 */
	if (!iexists) {
		if ((i = ino_new(m->m_buf)) == NULL) {
			msg_err(m->m_sender, strerror());
			return;
		}
	}

	/*
	 * If they want to use the existing file, set up the
	 * inode and let them go for it.  Note that this case
	 * MUST be iexists, or it would have been caught above.
	 */
	if (!(m->m_arg & ACC_CREATE)) {
		move_file(f, i, m->m_arg & ACC_WRITE);
	} else {
		/*
		 * Creation is desired.  If there's an existing file, free
		 * its storage.
		 */
		if (iexists) {
			blk_trunc(i);
			/* Marked bdirty() below */
		}

		/*
		 * Move pointers down to next free storage block
		 */
		i->i_start = 0;
		i->i_fsize = 0;
		ino_dirty(i);
		move_file(f, i, m->m_arg & ACC_WRITE);
	}
	
	ino_ref(i);
	m->m_buf = 0;
	m->m_buflen = 0;
	m->m_nseg = 0;
	m->m_arg1 = m->m_arg = 0;
	msg_reply(m->m_sender, m);
}


/*
 * bfs_close()
 *	Do closing actions on a file
 *
 * There is no FS_CLOSE message; this is entered in response to the
 * connection being terminated.
 */
void
bfs_close(struct file *f)
{
	/*
	 * A reference to the root dir needs no action
	 */
	if (f->f_inode->i_num == ROOTINODE)
		return;

	/*
	 * Free inode reference, decrement writers count
	 * if it was a writer.
	 */
	ino_deref(f->f_inode);
	if (f->f_write) {
		nwriters -= 1;
		bsync();
	}
}


/*
 * bfs_remove()
 *	Remove an entry in the current directory
 */
void
bfs_remove(struct msg *m, struct file *f)
{
	struct inode *i;

	/*
	 * Have to be in root dir, and have permission
	 */
	if (f->f_inode->i_num != ROOTINODE) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	if (!f->f_write) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Look up entry.  Bail if no such file.
	 */
	if ((i = ino_lookup(m->m_buf)) == NULL) {
		msg_err(m->m_sender, ESRCH);
		return;
	}



	if (i->i_refs > 0) {
		/*
		 * If we have more than one active reference to the inode
		 * we report a busy error
		 */
		msg_err(m->m_sender, EBUSY);
		return;
	}

	/*
	 * Zap the blocks
	 */
	blk_trunc(i);

	/*
	 * Flag it as an inactive entry
	 */
	i->i_name[0] = '\0';

	/*
	 * Flag the dir entry as dirty, and finish up.  Update
	 * the affected blocks to minimize damage from a crash.
	 */
	ino_dirty(i);
	bsync();
	ino_clear(i);

	/*
	 * Return success
	 */
	m->m_buflen = m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}


/*
 * do_rename()
 *	Actually perform the file rename
 *
 * This is much simpler than most fs's as we simply change the source inode's
 * name field - we have no links or subdirectories to worry about!
 */
static char *
do_rename(char *src, char *dest)
{
	struct inode *isrc, *idest;
	
	/*
	 * Look up the entries
	 */
	if ((isrc = ino_lookup(src)) == NULL) {
		/*
		 * If we can't find the source file fail!
		 */
		return ESRCH;
	}
	if ((idest = ino_lookup(dest)) != NULL) {
		/*
		 * If the destination exists, remove the existing file
		 */
		blk_trunc(idest);
		idest->i_name[0] = '\0';
		ino_dirty(idest);
		ino_clear(idest);
	}
	strncpy(isrc->i_name, dest, BFSNAMELEN);
	isrc->i_name[BFSNAMELEN - 1] = '\0';
	ino_dirty(isrc);

	return NULL;
}


/*
 * bfs_rename()
 *	Rename a file
 */
void
bfs_rename(struct msg *m, struct file *f)
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
	errstr = do_rename(f2->f_rename_msg.m_buf, m->m_buf);
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
 *	Cancel an ongoing file rename
 */
void
cancel_rename(struct file *f)
{
	(void)hash_delete(rename_pending, f->f_rename_id);
	f->f_rename_id = 0;
}
@


1.5
log
@Cleanup, add time stamp support, add rename() support
@
text
@d292 1
@


1.4
log
@Rev boot filesystem per work from Dave Hudson
@
text
@d5 1
a5 1
 * Last Update: 17th February 1994
d10 1
a10 1
 * Note that we don't allow subdirectories for BFS, which simplifies
d15 1
d23 3
a25 1
static int nwriters = 0;	/* # writers active */
d242 138
@


1.3
log
@Source reorg
@
text
@d2 5
a6 2
 * open.c
 *	Routines for opening, closing, creating  and deleting files
d8 2
d13 5
d19 1
a19 1
#include <sys/assert.h>
d22 1
a22 4
extern void *bdata();
extern struct inode *ifind();
extern char *strerror();
extern void blk_trunc(), bsync();
a23 1
static int nwriters = 0;	/* # writers active */
d27 1
a27 1
 *	Trasfer the current file of a struct file to the given inode
d32 1
a32 1
	ASSERT_DEBUG(f->f_inode == ROOTINO, "move_file: not root");
d35 1
a35 1
	if (f->f_write = writing)
d39 1
a46 2
	int newfile, off;
	void *handle;
d48 1
a48 2
	struct dirent *d;
	char *p;
d53 1
a53 1
	if (f->f_inode != ROOTINO) {
d90 4
a93 1
	newfile = dir_lookup(m->m_buf, &handle, &off);
d98 1
a98 1
	if (newfile && !(m->m_arg & ACC_CREATE)) {
d106 2
a107 2
	if (newfile) {
		if (dir_newfile(m->m_buf, &handle, &off)) {
a113 15
	 * Point to affected directory entry
	 */
	p = bdata(handle);
	d = (struct dirent *)(p + off);

	/*
	 * Get the inode for the directory entry
	 */
	i = ifind(d->d_inum);
	if (!i) {
		bfree(handle);
		msg_err(m->m_sender, ENOMEM);
	}

	/*
d116 1
a116 1
	 * MUST be !newfile, or it would have been caught above.
d120 9
a128 2
		goto success;
	}
d130 7
a136 7
	/*
	 * Creation is desired.  If there's an existing file, free
	 * its storage.
	 */
	if (!newfile) {
		blk_trunc(d);
		/* Marked bdirty() below */
d138 2
a139 9

	/*
	 * Move pointers down to next free storage block
	 */
	d->d_start = sblock->s_nextfree;
	d->d_len = 0;
	bdirty(handle);
	move_file(f, i, m->m_arg & ACC_WRITE);
success:
a144 3
	if (handle) {
		bfree(handle);
	}
d147 1
d161 1
a161 1
	if (!f->f_inode)
d168 1
a168 1
	ifree(f->f_inode);
d175 1
a182 3
	int off;
	void *handle;
	struct dirent *d;
d188 1
a188 1
	if (f->f_inode != ROOTINO) {
d200 1
a200 1
	if (dir_lookup(m->m_buf, &handle, &off)) {
d205 1
a205 4
	/*
	 * Get a handle on the entry
	 */
	d = (struct dirent *)((char *)bdata(handle) + off);
d207 7
a213 11
	/*
	 * Make sure nobody's using it
	 */
	i = ifind(d->d_inum);
	if (i) {
		if (i->i_refs > 1) {
			msg_err(m->m_sender, EBUSY);
			ifree(i);
			return;
		}
		ifree(i);
d219 1
a219 1
	blk_trunc(d);
d224 2
a225 4
	d->d_name[0] = '\0';
#ifdef DEBUG
	d->d_start = d->d_len = 0;
#endif
d230 1
a230 2
	bdirty(handle);
	bfree(handle);
d232 1
@


1.2
log
@GCC warning cleanup
@
text
@d8 1
a8 1
#include <bfs/bfs.h>
@


1.1
log
@Initial revision
@
text
@d15 1
a135 2
		extern void blk_trunc();

a167 2
	extern void bsync();

@
