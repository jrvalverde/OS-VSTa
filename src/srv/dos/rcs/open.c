head	1.13;
access;
symbols
	V1_3_1:1.9
	V1_3:1.8
	V1_2:1.7
	V1_1:1.7
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.13
date	95.02.04.05.57.42;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.11.16.19.38.53;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.10.23.17.42.37;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.09.23.20.36.37;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.20.21.05.43;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.03.23.21.57.33;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.48.09;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.10.06.23.30.01;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.20.21.26.46;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.19.21.42.41;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.07.21.15.31;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.16.22.46.12;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Front-line handling of directory lookup
@


1.13
log
@Fix optimization for flushing to dir entry only if length has
changed from dir entry's value.  On truncation of an existing file
dir entry was not updated (even in cache) to 0, so if file was then
regrown to the exact same size (easy for a.out's padded to 4K) we
wouldn't flush changes.
@
text
@/*
 * open.c
 *	Routines for opening, closing, creating  and deleting files
 */
#include "dos.h"
#include <sys/fs.h>
#include <sys/assert.h>
#include <std.h>
#include <sys/syscall.h>

/*
 * move_file()
 *	Transfer the current file of a struct file to the given node
 */
static void
move_file(struct file *f, struct node *n)
{
	deref_node(f->f_node);
	f->f_node = n;
	/* new node already ref'ed on lookup */
	f->f_pos = 0;
}

/*
 * dos_open()
 *	Main entry for processing an open message
 */
void
dos_open(struct msg *m, struct file *f)
{
	struct node *n;
	int newfile = 0;

	/*
	 * Have to be in dir to open down into a file
	 */
	if (f->f_node->n_type != T_DIR) {
		msg_err(m->m_sender, ENOTDIR);
		return;
	}

	/*
	 * Check for permission.  Write/create needs ACC_WRITE,
	 * changing permissions requires ACC_CHMOD.
	 */
	if (((m->m_arg & (ACC_WRITE|ACC_CREATE|ACC_DIR)) &&
		!(f->f_perm & ACC_WRITE))  ||
	    ((m->m_arg & ACC_CHMOD) && !(f->f_perm & ACC_CHMOD))) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Look up name
	 */
	n = dir_look(f->f_node, m->m_buf);

	/*
	 * Enforce DOS read-only bit
	 */
	if (n && !(n->n_mode & ACC_WRITE) && (m->m_arg & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * No such file--do they want to create?
	 */
	if (!n && !(m->m_arg & ACC_CREATE)) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * If it's a new file, allocate the entry now.
	 */
	if (!n) {
		n = dir_newfile(f, m->m_buf, m->m_arg & ACC_DIR);
		if (!n) {
			msg_err(m->m_sender, strerror());
			return;
		}
		newfile = 1;
	}

	/*
	 * If it's a symlink, don't let them open it
	 * unless they have ACC_SYM specified
	 */
	if ((n->n_type == T_SYM) && !(m->m_arg & ACC_SYM)) {
		msg_err(m->m_sender, ESYMLINK);
		deref_node(n);
		return;
	}

	/*
	 * If they want to use the existing file, set up the
	 * node and let them go for it.  Note that this case
	 * MUST be !newfile, or it would have been caught above.
	 */
	if (!(m->m_arg & ACC_CREATE)) {
		if (!newfile && (m->m_arg & ACC_WRITE)) {
			/*
			 * When an existing file is opened for modification,
			 * mark its new modification date/time.  This makes
			 * commands like "touch" work.
			 */
			dir_timestamp(n, 0);
		}
		goto success;
	}

	/*
	 * Creation is desired.  If there's an existing file, free
	 * its storage.
	 */
	if (!newfile) {
		/*
		 * Can't rewrite a directory like this
		 */
		if (n->n_type == T_DIR) {
			msg_err(m->m_sender, EEXIST);
			deref_node(n);
			return;
		}
		clust_setlen(n->n_clust, 0L);
		n->n_len = 0;
		n->n_flags |= N_DIRTY;
		dir_setlen(n);
	}
success:
	move_file(f, n);
	m->m_nseg = 0;
	m->m_arg1 = m->m_arg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * dos_close()
 *	Do closing actions on a file
 */
void
dos_close(struct file *f)
{
	deref_node(f->f_node);
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

	unhash(rootport, unhash_fid);
	_exit(0);
}

/*
 * dos_remove()
 *	Remove an entry in the current directory
 */
void
dos_remove(struct msg *m, struct file *f)
{
	struct node *n;

	/*
	 * Need a buffer
	 */
	if (m->m_nseg != 1) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Have to have write permission
	 */
	if (!(f->f_perm & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Have to be in a directory
	 */
	if (f->f_node->n_type != T_DIR) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Look up entry.  Bail if no such file.
	 */
	n = dir_look(f->f_node, m->m_buf);
	if (n == 0) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * Try unhashing if it might be the only other reference
	 */
	if (n->n_refs == 2) {
		extern uint inum();

		/*
		 * Since a closing portref needs to handshake
		 * with the server, use a child thread to do
		 * the dirty work.
		 */
		unhash_fid = inum(n);
		(void)tfork(do_unhash);

		/*
		 * Release our ref and tell the requestor he
		 * might want to try again.
		 */
		deref_node(n);
		msg_err(m->m_sender, EAGAIN);
		return;
	}

	/*
	 * We must be only access
	 */
	if (n->n_refs > 1) {
		deref_node(n);
		msg_err(m->m_sender, EBUSY);
		return;
	}

	/*
	 * Directories--only allowed when empty
	 */
	if (n->n_type == T_DIR) {
		if (!dir_empty(n)) {
			deref_node(n);
			msg_err(m->m_sender, EBUSY);
			return;
		}
	}

	/*
	 * Ask dir routines to remove
	 */
	dir_remove(n);

	/*
	 * Throw away his storage, mark him as already cleaned
	 */
	clust_setlen(n->n_clust, 0L);
	n->n_flags |= N_DEL;

	/*
	 * Done with node
	 */
	deref_node(n);
	sync();

	/*
	 * Return success
	 */
	m->m_buflen = m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}
@


1.12
log
@Only set modification time on open for writing/truncation.  Fix
setting of mtime so mtime isn't always overwritten when mod time
written on last close.
@
text
@d105 2
a106 1
			 * mark its new modification date/time.
a126 1
		dir_timestamp(n, 0);
d129 1
@


1.11
log
@Implement read-only attribute.  Also fix flushing of
symlink type to disk.
@
text
@d102 7
d126 1
@


1.10
log
@Add symlink support
@
text
@a34 8
	 * Need a buffer
	 */
	if (m->m_nseg != 1) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
d43 2
a44 1
	 * Check for permission
d46 5
a50 8
	if (m->m_arg & (ACC_WRITE|ACC_CREATE|ACC_DIR)) {
		/*
		 * Insufficient priveleges
		 */
		if ((f->f_perm & ACC_WRITE) == 0) {
			msg_err(m->m_sender, EPERM);
			return;
		}
d57 8
@


1.9
log
@Fix flushing dirty nodes out on close()
@
text
@d89 10
@


1.8
log
@Fix -Wall warnings
@
text
@d112 1
@


1.7
log
@Source reorg
@
text
@d9 1
a10 2
extern struct node *rootdir;

a151 2
	void *handle;
	struct directory *d;
@


1.6
log
@Need to flush out the FAT once we've added those new free blocks
@
text
@d5 1
a5 1
#include <dos/dos.h>
@


1.5
log
@Try unhash()'ing busy file on unlink()
@
text
@d247 1
@


1.4
log
@Fix n_len on truncation
@
text
@d132 14
d186 23
@


1.3
log
@Add N_DEL flag for deleted node; fiddle when the directory
entry is removed; we need the first cluster # from the dir
entry being deleted so we can use it as the key inthe parent.
@
text
@d112 1
@


1.2
log
@Apply sanity to buffer before using
@
text
@d195 6
a200 1
	 * Throw away his storage
d203 1
d206 1
a206 1
	 * Ask dir routines to remove
a207 1
	dir_remove(n);
@


1.1
log
@Initial revision
@
text
@d36 8
d140 8
@
