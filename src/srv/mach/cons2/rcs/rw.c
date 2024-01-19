head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3;
locks; strict;
comment	@ * @;


1.3
date	94.02.28.19.17.00;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.02.28.04.52.37;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.02.28.04.29.46;	author vandys;	state Exp;
branches;
next	;


desc
@Read/write handling for keyboard side
@


1.3
log
@Fix read queue handling (per-screen)
@
text
@/*
 * rw.c
 *	Reads and writes to the keyboard device
 *
 * Well, actually just reads, since it's a keyboard.  But r.c looked
 * a little strange.
 */
#include "cons.h"
#include <sys/assert.h>
#include <sys/seg.h>

/*
 * kbd_read()
 *	Post a read to the keyboard
 *
 * m_arg specifies how much they want at most.  A value of 0 is
 * special; we will send 0 or more bytes, but will not wait for
 * more bytes.  It is, in a sense, a non-blocking read.
 */
void
kbd_read(struct msg *m, struct file *f)
{
	struct screen *s = &screens[f->f_screen];

	/*
	 * Handle easiest first--non-blocking read, no data
	 */
	if ((m->m_arg == 0) && (s->s_nbuf == 0)) {
		m->m_buflen = m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Next easiest: non-blocking read, give'em what we have
	 */
	if (m->m_arg == 0) {
		m->m_buf = &s->s_buf[s->s_tl];

		/*
		 * If all the data's in a row, send with just the
		 * buffer.
		 */
		if (s->s_hd > s->s_tl) {
			m->m_arg = m->m_buflen = s->s_hd - s->s_tl;
			m->m_nseg = 1;
		} else {
			/*
			 * If not, send the second extent using
			 * a segment.
			 */
			m->m_buflen = KEYBD_MAXBUF - s->s_tl;
			m->m_nseg = 2;
			m->m_seg[1] = seg_create(s->s_buf, s->s_hd);
			m->m_arg = m->m_buflen + s->s_hd;
		}
		s->s_tl = s->s_hd;
		s->s_nbuf = 0;
		m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * If we have data, give them up to what they want
	 */
	if (s->s_nbuf > 0) {
		/*
		 * See how much we can get in one run
		 */
		m->m_nseg = 1;
		m->m_buf = &s->s_buf[s->s_tl];
		if (s->s_hd > s->s_tl) {
			m->m_buflen = s->s_hd - s->s_tl;
		} else {
			m->m_buflen = KEYBD_MAXBUF - s->s_tl;
		}

		/*
		 * Cap at how much they want
		 */
		if (m->m_buflen > m->m_arg) {
			m->m_buflen = m->m_arg;
		}

		/*
		 * Update tail pointer
		 */
		s->s_nbuf -= m->m_buflen;
		s->s_tl += m->m_buflen;
		if (s->s_tl >= KEYBD_MAXBUF) {
			s->s_tl -= KEYBD_MAXBUF;
		}

		/*
		 * Send back data
		 */
		m->m_arg = m->m_buflen;
		m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
	 * Last but not least, queue the I/O until we can complete
	 * it.
	 */
	f->f_readcnt = m->m_arg;
	f->f_sender = m->m_sender;
	ll_insert(&s->s_readers, f);
}

/*
 * abort_read()
 *	Remove our guy from the waiting queue for data
 */
void
abort_read(struct file *f)
{
	struct llist *l;
	struct screen *s;

	s = &screens[f->f_screen];
	for (l = s->s_readers.l_forw; l != &s->s_readers; l = l->l_forw) {
		if (l->l_data == f) {
			ll_delete(l);
			break;
		}
	}
	ASSERT_DEBUG(l != &s->s_readers, "abort_read: missing tran");
}

/*
 * kbd_enqueue()
 *	Get a new character to enqueue
 */
void
kbd_enqueue(struct screen *s, uint c)
{
	char buf[4];
	struct msg m;
	struct file *f;
	struct llist *l;

	/*
	 * If there's a waiter, just let'em have it
	 */
	l = s->s_readers.l_forw;
	if (l != &s->s_readers) {
		ASSERT_DEBUG(s->s_nbuf == 0, "kbd: waiters with data");

		/*
		 * Extract the waiter from the list
		 */
		f = l->l_data;
		ll_delete(l);

		/*
		 * Put char in buffer, send message
		 */
		buf[0] = c;	/* Don't assume endianness in an int */
		m.m_buf = buf;
		m.m_arg = m.m_buflen = sizeof(char);
		m.m_nseg = 1;
		m.m_arg1 = 0;
		msg_reply(f->f_sender, &m);

		/*
		 * Clear pending transaction
		 */
		f->f_readcnt = 0;
#ifdef DEBUG
		f->f_sender = 0;
#endif
		return;
	}

	/*
	 * If there's too much queued data, ignore
	 */
	if (s->s_nbuf >= KEYBD_MAXBUF) {
		return;
	}

	/*
	 * Add it to the queue
	 */
	s->s_buf[s->s_hd] = c;
	s->s_hd += 1;
	s->s_nbuf += 1;
	if (s->s_hd >= KEYBD_MAXBUF) {
		s->s_hd = 0;
	}
}
@


1.2
log
@Modify keyboard handling to do multiple virtual input queues
@
text
@a8 2
#include <llist.h>
#include <mach/kbd.h>
d110 1
a110 1
	ll_insert(&read_q, f);
d121 1
d123 2
a124 1
	for (l = read_q.l_forw; l != &read_q; l = l->l_forw) {
d130 1
a130 1
	ASSERT_DEBUG(l != &read_q, "abort_read: missing tran");
d148 2
a149 2
	l = read_q.l_forw;
	if (l != &read_q) {
@


1.1
log
@Initial revision
@
text
@d8 1
a8 1
#include <sys/msg.h>
a14 13
 * A circular buffer for keystrokes
 */
static char key_buf[KEYBD_MAXBUF];
static uint key_hd = 0,
	key_tl = 0;
uint key_nbuf = 0;

/*
 * Our queue for I/O's pending
 */
static struct llist read_q;

/*
d25 2
d30 1
a30 1
	if ((m->m_arg == 0) && (key_nbuf == 0)) {
d40 1
a40 1
		m->m_buf = &key_buf[key_tl];
d46 2
a47 2
		if (key_hd > key_tl) {
			m->m_arg = m->m_buflen = key_hd-key_tl;
d54 1
a54 1
			m->m_buflen = KEYBD_MAXBUF-key_tl;
d56 2
a57 2
			m->m_seg[1] = seg_create(key_buf, key_hd);
			m->m_arg = m->m_buflen + key_hd;
d59 2
a60 2
		key_tl = key_hd;
		key_nbuf = 0;
d69 1
a69 1
	if (key_nbuf > 0) {
d74 3
a76 3
		m->m_buf = &key_buf[key_tl];
		if (key_hd > key_tl) {
			m->m_buflen = key_hd-key_tl;
d78 1
a78 1
			m->m_buflen = KEYBD_MAXBUF-key_tl;
d91 4
a94 4
		key_nbuf -= m->m_buflen;
		key_tl += m->m_buflen;
		if (key_tl >= KEYBD_MAXBUF) {
			key_tl -= KEYBD_MAXBUF;
d110 1
a110 1
	f->f_count = m->m_arg;
a115 10
 * kbd_init()
 *	Set up our read queue structure once
 */
void
kbd_init()
{
	ll_init(&read_q);
}

/*
d138 1
a138 1
kbd_enqueue(uint c)
d150 1
a150 1
		ASSERT_DEBUG(key_nbuf == 0, "kbd: waiters with data");
d171 1
a171 1
		f->f_count = 0;
d181 1
a181 1
	if (key_nbuf >= KEYBD_MAXBUF) {
d188 5
a192 5
	key_buf[key_hd] = c;
	key_hd += 1;
	key_nbuf += 1;
	if (key_hd >= KEYBD_MAXBUF) {
		key_hd = 0;
@
