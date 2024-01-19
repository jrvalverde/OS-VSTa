head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.2
date	93.11.16.02.48.35;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.12.12;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of read/write of namer nodes
@


1.2
log
@Source reorg
@
text
@/*
 * rw.c
 *	Read and write functions
 *
 * Our objects only hold numbers, which simplifies things
 * quite a bit.
 */
#define _NAMER_H_INTERNAL
#include <llist.h>
#include <sys/namer.h>
#include <sys/fs.h>

extern void *malloc();
extern char *strerror();

/*
 * namer_write()
 *	Write to an open file
 */
void
namer_write(struct msg *m, struct file *f)
{
	struct node *n = f->f_node;
	char buf[16];

	/*
	 * Can only write to a true file, and only if open for writing.
	 */
	if ((n->n_internal) || !(f->f_mode & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Have to have buffer, make sure it's null-terminated
	 */
	if (m->m_buflen > (sizeof(buf)-1)) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	bcopy(m->m_buf, buf, m->m_buflen);
	buf[m->m_buflen] = '\0';

	/*
	 * Sanity on buffer, calculate value, update node
	 */
	n->n_port = (port_name)atoi(buf);

	m->m_buflen = m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * namer_readdir()
 *	Do reads on directory entries
 */
static void
namer_readdir(struct msg *m, struct file *f)
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
	l = n->n_elems.l_forw;
	while (x < f->f_pos) {
		/*
		 * Check "EOF"
		 */
		if (l == &n->n_elems)
			break;

		/*
		 * Advance
		 */
		l = l->l_forw;
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
		l = l->l_forw;
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
 * namer_read()
 *	Read bytes out of the current file or directory
 *
 * Internal nodes get their own routine.
 */
void
namer_read(struct msg *m, struct file *f)
{
	int cnt;
	char buf[8];
	struct node *n = f->f_node;

	/*
	 * Directory
	 */
	if (n->n_internal) {
		namer_readdir(m, f);
		return;
	}

	/*
	 * Generate our "contents"
	 */
	sprintf(buf, "%d", n->n_port);

	/*
	 * Calculate # bytes to get
	 */
	cnt = m->m_arg;
	if (cnt > (strlen(buf) - f->f_pos))
		cnt = strlen(buf) - f->f_pos;

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
	m->m_buf = buf+f->f_pos;
	m->m_arg = m->m_buflen = cnt;
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	f->f_pos += cnt;
}
@


1.1
log
@Initial revision
@
text
@d9 2
a10 2
#include <lib/llist.h>
#include <namer/namer.h>
@
