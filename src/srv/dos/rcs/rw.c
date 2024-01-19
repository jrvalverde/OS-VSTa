head	1.8;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.7
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.8
date	94.03.23.21.58.15;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.24.19.58.25;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.48.09;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.17.00.25.50;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.22.15.44.55;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.22.14.49.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.21.45.21;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Read/write
@


1.8
log
@Fix -Wall warnigns
@
text
@/*
 * rw.c
 *	Routines for operating on the data in a file
 */
#include <sys/fs.h>
#include "dos.h"
#include <std.h>
#include <ctype.h>
#include <sys/assert.h>

/*
 * do_write()
 *	Local routine to loop over a buffer and write it to a file
 *
 * Returns 0 on success, 1 on error.
 */
static int
do_write(struct clust *c, uint pos, char *buf, uint cnt)
{
	uint bufoff, step, blk, boff;
	void *handle;

	/*
	 * Loop across each block, putting our data into place
	 */
	bufoff = 0;
	while (cnt > 0) {
		/*
		 * Calculate how much to take out of current block
		 */
		boff = pos & (BLOCKSIZE-1);
		step = BLOCKSIZE - boff;
		if (step >= cnt) {
			step = cnt;
		}

		/*
		 * Map current block
		 */
		blk = pos / BLOCKSIZE;
		handle = bget(c->c_clust[blk]);
		if (!handle) {
			return(1);
		}

		/*
		 * Copy data, mark buffer dirty, free it
		 */
		memcpy((char *)bdata(handle)+boff, buf+bufoff, step);
		bdirty(handle);
		bfree(handle);

		/*
		 * Advance counters
		 */
		pos += step;
		bufoff += step;
		cnt -= step;
	}
	return(0);
}

/*
 * dos_write()
 *	Write to an open file
 */
void
dos_write(struct msg *m, struct file *f)
{
	struct node *n = f->f_node;
	ulong newlen;

	/*
	 * Can only write to a true file, and only if open for writing.
	 */
	if ((n->n_type == T_DIR) || !(f->f_perm & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * See if the file's going to be able to hold all the data.  We
	 * do not necessarily need to allocate space if we're rewriting
	 * an existing file.
	 */
	newlen = f->f_pos + m->m_buflen;
	if (newlen > n->n_len) {
		if (clust_setlen(n->n_clust, newlen)) {
			msg_err(m->m_sender, ENOSPC);
			return;
		}
		n->n_len = newlen;
	}
	n->n_flags |= N_DIRTY;

	/*
	 * Copy out the buffer
	 */
	if (do_write(n->n_clust, f->f_pos, m->m_buf, m->m_buflen)) {
		msg_err(m->m_sender, strerror());
		return;
	}
	m->m_arg = m->m_buflen;
	f->f_pos += m->m_buflen;
	m->m_buflen = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * pack_name()
 *	Pack a DOS name into a UNIX-ish format
 */
void
pack_name(char *name, char *ext, char *file)
{
	char *p;

	for (p = name; (p < name+8) && (*p != ' '); ++p) {
		*file++ = tolower(*p);
	}
	p = ext;
	ASSERT_DEBUG(*p, "pack_name: null in filename");
	if (*p != ' ') {
		*file++ = '.';
		for ( ; (p < ext+3) && (*p != ' '); ++p) {
			*file++ = tolower(*p);
		}
	}
	*file = '\0';
}

/*
 * dos_readdir()
 *	Do reads on directory entries
 */
static void
dos_readdir(struct msg *m, struct file *f)
{
	char *buf;
	uint len, x;
	struct directory d;
	struct node *n = f->f_node;
	char file[14];

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
	 * Skip "." and "..", which exist only in non-root directories
	 */
	if ((n != rootdir) && (f->f_pos == 0)) {
		f->f_pos = 2;
	}

	/*
	 * Assemble as many names as will fit
	 */
	for (x = 0; x < len; ) {
		uint c;

		/*
		 * Look at the slot at f_pos.  For reads of directories
		 * f_pos is simply the struct directory index.  Leave
		 * loop on failure, presumably from EOF.
		 */
		if (dir_copy(n, f->f_pos++, &d)) {
			break;
		}

		/*
		 * Leave after last entry, skip deleted entries
		 */
		c = (d.name[0] & 0xFF);
		if (!c) {
			break;
		}
		if (c == 0xe5) {
			continue;
		}

		/*
		 * If the next entry won't fit, back up the file
		 * position and return what we have.
		 */
		pack_name(d.name, d.ext, file);
		if ((x + strlen(file) + 1) >= len) {
			f->f_pos -= 1;
			break;
		}

		/*
		 * Add entry and a newline
		 */
		strcat(buf+x, file);
		strcat(buf+x, "\n");
		x += (strlen(file)+1);
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
 * dos_read()
 *	Read bytes out of the current file or directory
 *
 * Directories get their own routine.
 */
void
dos_read(struct msg *m, struct file *f)
{
	int x, step, cnt, blk, boff;
	struct node *n = f->f_node;
	void *handle;
	char *buf;
	struct clust *c = n->n_clust;

	/*
	 * Directory
	 */
	if (n->n_type == T_DIR) {
		dos_readdir(m, f);
		return;
	}

	/*
	 * EOF?
	 */
	if (f->f_pos >= n->n_len) {
		m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
		return;
	}
	ASSERT_DEBUG(c->c_clust, "dos_read: len !clust");
	ASSERT_DEBUG(c->c_nclust > 0, "dos_read: clust !nclust");

	/*
	 * Calculate # bytes to get
	 */
	cnt = m->m_arg;
	if (cnt > (n->n_len - f->f_pos)) {
		cnt = n->n_len - f->f_pos;
	}

	/*
	 * Get a buffer big enough to do the job
	 * XXX user scatter-gather, this is a waste
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
		boff = f->f_pos & (BLOCKSIZE-1);
		step = BLOCKSIZE - boff;
		if (step >= (cnt-x)) {
			step = (cnt-x);
		}

		/*
		 * Map current block
		 */
		blk = f->f_pos / BLOCKSIZE;
		ASSERT_DEBUG(blk < c->c_nclust, "dos_read: bad blk");
		handle = bget(c->c_clust[blk]);
		if (!handle) {
			free(buf);
			msg_err(m->m_sender, strerror());
			return;
		}
		bcopy((char *)bdata(handle)+boff, buf+x, step);
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
	m->m_nseg = (cnt ? 1 : 0);
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	free(buf);
}
@


1.7
log
@If file has no extension, map to "name", not "name."
@
text
@d17 1
a17 1
static
a69 1
	void *handle;
@


1.6
log
@Source reorg
@
text
@d122 7
a128 3
	*file++ = '.';
	for (p = ext; (p < ext+3) && (*p != ' '); ++p) {
		*file++ = tolower(*p);
d130 1
a130 1
	*file++ = '\0';
@


1.5
log
@A small mistake with the treatment of the "cnt" variable; it would
cause us to walk off the end of the user buffer.
@
text
@d6 1
a6 1
#include <dos/dos.h>
@


1.4
log
@Get rid of stray printf
@
text
@d18 1
a18 1
do_write(struct clust *c, int pos, char *buf, int cnt)
d20 1
a20 1
	int x, step, blk, boff;
d26 2
a27 1
	for (x = 0; x < cnt; ) {
d43 1
a43 1
			return 1;
d45 5
a49 2
		memcpy((char *)bdata(handle)+boff, buf+x, step);
		pos += step;
d54 1
a54 1
		 * Advance to next chunk
d56 3
a58 1
		x += step;
@


1.3
log
@Avoid dotdot directories; we handle that stuff in lookup code
@
text
@a300 1
	printf("Send back %d bytes\n", cnt);
@


1.2
log
@Flag node dirty when we write
@
text
@d151 7
@


1.1
log
@Initial revision
@
text
@d89 1
@
