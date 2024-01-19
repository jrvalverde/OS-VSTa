head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.5
date	94.10.05.18.32.29;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.49.30;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.05.06.23.20.50;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.23.19.48.52;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.23.19.10.15;	author vandys;	state Exp;
branches;
next	;


desc
@Open/lookup handling
@


1.5
log
@Merge in Dave Hudson's bug fixes, especially "foo | less"
function.
@
text
@/*
 * open.c
 *	Routines for opening, closing, creating  and deleting files
 *
 * Note that we don't allow subdirectories for BFS, which simplifies
 * things.
 */
#include "pipe.h"
#include <hash.h>
#include <std.h>
#include <sys/assert.h>

extern struct llist files;	/* All files in FS */

/*
 * dir_lookup()
 *	Look up name in list of all files in this FS
 */
struct pipe *
dir_lookup(char *name)
{
	struct llist *l;
	struct pipe *o;
	ulong x;

	if (sscanf(name, "%ld", &x) != 1) {
		return(0);
	}
	o = (struct pipe *)x;
	for (l = LL_NEXT(&files); l != &files; l = LL_NEXT(l)) {
		if (o == l->l_data) {
			return(o);
		}
	}
	return(0);
}

/*
 * freeup()
 *	Free all memory associated with a file
 */
static void
freeup(struct pipe *o)
{
	if (o == 0) {
		return;
	}
	if (o->p_entry) {
		ll_delete(o->p_entry);
	}
	free(o);
}

/*
 * dir_newfile()
 *	Create new entry in filesystem
 */
struct pipe *
dir_newfile(struct file *f, char *name)
{
	struct pipe *o;
	struct prot *p;

	/*
	 * Get new node
	 */
	o = malloc(sizeof(struct pipe));
	if (o == 0) {
		return(0);
	}
	bzero(o, sizeof(struct pipe));
	ll_init(&o->p_readers);
	ll_init(&o->p_writers);

	/*
	 * Insert in dir chain
	 */
	o->p_entry = ll_insert(&files, o);
	if (o->p_entry == 0) {
		freeup(o);
		return(0);
	}

	/*
	 * Use 0'th perm as our prot, require full match
	 */
	p = &o->p_prot;
	bzero(p, sizeof(*p));
	p->prot_len = PERM_LEN(&f->f_perms[0]);
	bcopy(f->f_perms[0].perm_id, p->prot_id, PERMLEN);
	p->prot_bits[p->prot_len-1] =
		ACC_READ|ACC_WRITE|ACC_CHMOD;
	o->p_owner = f->f_perms[0].perm_uid;
	return(o);
}

/*
 * pipe_open()
 *	Main entry for processing an open message
 */
void
pipe_open(struct msg *m, struct file *f)
{
	struct pipe *o;
	uint x;

	/*
	 * Have to be in root dir to open down into a file
	 */
	if (f->f_file) {
		msg_err(m->m_sender, ENOTDIR);
		return;
	}

	/*
	 * No subdirs in a pipe filesystem
	 */
	if (m->m_arg & ACC_DIR) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Look up name
	 */
	if ((m->m_buflen != 2) || (((char *)(m->m_buf))[0] != '#')) {
		o = dir_lookup(m->m_buf);
		if (!o) {
			msg_err(m->m_sender, ESRCH);
			return;
		}
	} else {
		o = 0;
	}

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
		 * Failure?
		 */
		if ((o = dir_newfile(f, m->m_buf)) == 0) {
			msg_err(m->m_sender, ENOMEM);
			return;
		}

		/*
		 * Move to new node
		 */
		f->f_file = o; o->p_refs += 1;
		ASSERT_DEBUG(o->p_refs > 0, "pipe_open: nwrite overflow");
		f->f_perm = ACC_READ | ACC_WRITE | ACC_CHMOD;
		o->p_nwrite += 1;
		ASSERT_DEBUG(o->p_nwrite > 0, "pipe_open: nwrite overflow");
		m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Check whether the pipe's really closed and we're simply allowing
	 * a writer to purge it's buffers
	 */
	if (o->p_nread == PIPE_CLOSED_FOR_READS) {
		msg_err(m->m_sender, EPIPE);
		return;
	}

	/*
	 * Check permission
	 */
	x = perm_calc(f->f_perms, f->f_nperm, &o->p_prot);
	if ((m->m_arg & x) != m->m_arg) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Move to this file
	 */
	f->f_file = o; o->p_refs += 1;
	ASSERT_DEBUG(o->p_refs > 0, "pipe_open: overflow");
	f->f_perm = m->m_arg | (x & ACC_CHMOD);
	if (m->m_arg & ACC_WRITE) {
		o->p_nwrite += 1;
		ASSERT_DEBUG(o->p_nwrite > 0, "pipe_open: overflow");
	} else {
		o->p_nread += 1;
		ASSERT_DEBUG(o->p_nread > 0, "pipe_open: overflow");
	}
	m->m_nseg = m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * pipe_close()
 *	Do closing actions on a file
 */
void
pipe_close(struct file *f)
{
	struct pipe *o;

	/*
	 * No ref count on dir
	 */
	o = f->f_file;
	if (o == 0) {
		return;
	}

	/*
	 * Free a ref.  No more clients--free node.
	 */
	ASSERT_DEBUG(o->p_refs > 0, "pipe_close: underflow");
	o->p_refs -= 1;
	if (o->p_refs == 0) {
		freeup(o);
		return;
	}

	/*
	 * Are we a reader or writer?
	 */
	if ((f->f_perm & ACC_WRITE) == 0) {
		/*
		 * We're a reader - if we're the last reader stop all of
		 * the pending writers
		 */
		ASSERT_DEBUG(o->p_nread > 0, "pipe_close: nread underflow");
		o->p_nread -= 1;
		if (o->p_nread > 0) {
			return;
		}
		if (o->p_nwrite == 0) {
			return;
		}
		o->p_nread = PIPE_CLOSED_FOR_READS;
		while (!LL_EMPTY(&o->p_writers)) {
			struct msg *m;
			struct file *f2;

			f2 = LL_NEXT(&o->p_writers)->l_data;
			ASSERT_DEBUG(f2->f_q, "pipe_close: !busy writers");
			ll_delete(f2->f_q);
			f2->f_q = 0;
			m = &f2->f_msg;
			msg_err(m->m_sender, EPIPE);
		}
	} else {
		/*
		 * If this is the close of the last writer, bomb
		 * all of the pending readers
		 */
		ASSERT_DEBUG(o->p_nwrite > 0, "pipe_close: nwrite underflow");
		o->p_nwrite -= 1;
		if (o->p_nwrite > 0) {
			return;
		}
		while (!LL_EMPTY(&o->p_readers)) {
			struct msg *m;
			struct file *f2;

			f2 = LL_NEXT(&o->p_readers)->l_data;
			ASSERT_DEBUG(f2->f_q, "pipe_close: !busy readers");
			ll_delete(f2->f_q);
			f2->f_q = 0;
			m = &f2->f_msg;
			m->m_arg = m->m_arg1 = m->m_nseg = 0;
			msg_reply(m->m_sender, m);
		}
	}
}
@


1.4
log
@Source reorg
@
text
@d160 2
a161 2
		ASSERT_DEBUG(o->p_refs > 0, "pipe_open: overflow");
		f->f_perm = ACC_READ|ACC_WRITE|ACC_CHMOD;
d163 1
a163 1
		ASSERT_DEBUG(o->p_nwrite > 0, "pipe_open: overflow");
d170 9
d196 3
d232 1
a232 1
	 * If this is a reader-side, no further action to take
d235 37
a271 2
		return;
	}
d273 8
a280 19
	/*
	 * Close of last writer--bomb all pending readers
	 */
	ASSERT_DEBUG(o->p_nwrite > 0, "pipe_close: underflow");
	o->p_nwrite -= 1;
	if (o->p_nwrite > 0) {
		return;
	}
	while (!LL_EMPTY(&o->p_readers)) {
		struct msg *m;
		struct file *f2;

		f2 = LL_NEXT(&o->p_readers)->l_data;
		ASSERT_DEBUG(f2->f_q, "pipe_close: !busy");
		ll_delete(f2->f_q);
		f2->f_q = 0;
		m = &f2->f_msg;
		m->m_arg = m->m_arg1 = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
@


1.3
log
@Fix reference counting and add debug sanity checks for reference
counts.
@
text
@d8 2
a9 3
#include <pipe/pipe.h>
#include <lib/llist.h>
#include <lib/hash.h>
@


1.2
log
@Add EOF on close of last writer
@
text
@a74 1
	o->p_nwrite = 1;
d161 1
d163 2
d183 1
d185 4
d213 1
d221 7
d230 1
@


1.1
log
@Initial revision
@
text
@d75 1
d195 35
a229 5
	if (o = f->f_file) {
		o->p_refs -= 1;
		if (o->p_refs == 0) {
			freeup(o);
		}
@
