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
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.06.27.17.40.39;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.17.05.13;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.05.16.02.28;	author vandys;	state Exp;
branches;
next	;


desc
@Reading/writing of nodes, plus reading directories
@


1.4
log
@Source reorg
@
text
@/*
 * rw.c
 *	Read and write functions
 *
 * Our objects hold strings of up to a certain length, defined below.
 */
#include "env.h"
#include <sys/fs.h>
#include <std.h>

#define MAX_STRING (1024)	/* 1K should be enough? */

/*
 * env_write()
 *	Write to an open file
 */
void
env_write(struct msg *m, struct file *f, uint len)
{
	struct node *n = f->f_node;
	uint newlen, oldlen;
	char *buf;

	/*
	 * Can only write to a true file, and only if open for writing.
	 */
	if (DIR(n) || !(f->f_mode & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Have to have buffer, make sure it's null-terminated
	 */
	newlen = f->f_pos + len + 1;
	oldlen = strlen(n->n_val->s_val)+1;
	if (newlen >= MAX_STRING) {
		msg_err(m->m_sender, E2BIG);
		return;
	}
	if (newlen < oldlen) {
		newlen = oldlen;
	}
	if ((buf = malloc(newlen)) == 0) {
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Transfer old contents, tack new stuff on end
	 */
	strcpy(buf, n->n_val->s_val);
	seg_copyin(m->m_seg, m->m_nseg, buf + f->f_pos, len);
	buf[newlen-1] = '\0';

	/*
	 * Free old string storage, put ours in its place
	 */
	free(n->n_val->s_val);
	n->n_val->s_val = buf;

	/*
	 * Success
	 */
	m->m_buflen = m->m_arg1 = m->m_nseg = 0;
	m->m_arg = len;
	msg_reply(m->m_sender, m);
	f->f_pos += len;
}

/*
 * env_readdir()
 *	Do reads on directory entries
 */
static void
env_readdir(struct msg *m, struct file *f)
{
	int x, len;
	char *buf;
	struct node *n = f->f_node;
	struct llist *l;

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

	/*
	 * Find next directory entry.  We use our position
	 * as a count for scanning forward, and consider
	 * EOF to happen when we wrap to our start.
	 */
	x = 0;
	buf[0] = '\0';
	l = LL_NEXT(&n->n_elems);
	while (x < f->f_pos) {
		/*
		 * Check "EOF"
		 */
		if (l == &n->n_elems) {
			break;
		}

		/*
		 * Advance
		 */
		l = LL_NEXT(l);
		++x;
	}

	/*
	 * Assemble as many names as we have and will fit
	 */
	for (x = 0; x < len; ) {
		struct node *n2;


		/*
		 * If EOF, return what we have
		 */
		if (l == &n->n_elems) {
			break;
		}

		/*
		 * If the next entry won't fit, back up the file
		 * position and return what we have.
		 */
		n2 = l->l_data;
		if ((x + strlen(n2->n_name) + 1) >= len) {
			break;
		}

		/*
		 * Add entry and a newline
		 */
		strcat(buf+x, n2->n_name);
		strcat(buf+x, "\n");
		x += (strlen(n2->n_name)+1);
		f->f_pos += 1;
		l = LL_NEXT(l);
	}

	/*
	 * Send back results
	 */
	m->m_buf = buf;
	m->m_arg = m->m_buflen = x;
	m->m_nseg = (x ? 1 : 0);
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	free(buf);
}

/*
 * env_read()
 *	Read bytes out of the current file or directory
 *
 * Internal nodes get their own routine.
 */
void
env_read(struct msg *m, struct file *f, uint len)
{
	int cnt;
	char *buf;
	struct node *n = f->f_node;

	/*
	 * Directory
	 */
	if (DIR(n)) {
		env_readdir(m, f);
		return;
	}

	/*
	 * Generate our "contents"
	 */
	buf = n->n_val->s_val;
	buf += f->f_pos;

	/*
	 * Calculate # bytes to get
	 */
	cnt = m->m_arg;
	if (cnt > strlen(buf)) {
		cnt = strlen(buf);
	}

	/*
	 * EOF?
	 */
	if (cnt <= 0) {
		m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Send back reply
	 */
	m->m_buf = buf;
	m->m_arg = m->m_buflen = cnt;
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	f->f_pos += cnt;
}
@


1.3
log
@Fix off-by-one termination placement
@
text
@d7 1
a7 2
#include <lib/llist.h>
#include <env/env.h>
@


1.2
log
@Tidy up list traversal
@
text
@d54 2
a55 3
	seg_copyin(m->m_seg, m->m_nseg,
		buf + f->f_pos, newlen - f->f_pos);
	buf[newlen] = '\0';
@


1.1
log
@Initial revision
@
text
@d19 1
a19 1
env_write(struct msg *m, struct file *f, int len)
d22 1
a22 1
	uint newlen, oldlen, x;
a23 1
	struct string *str;
d28 1
a28 1
	if ((n->n_internal) || !(f->f_mode & ACC_WRITE)) {
d105 1
a105 1
	l = n->n_elems.l_forw;
d110 1
a110 1
		if (l == &n->n_elems)
d112 1
d117 1
a117 1
		l = l->l_forw;
d151 1
a151 1
		l = l->l_forw;
d172 1
a172 1
env_read(struct msg *m, struct file *f, int len)
d181 1
a181 1
	if (n->n_internal) {
@
