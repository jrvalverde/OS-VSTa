head	1.2;
access;
symbols;
locks; strict;
comment	@ * @;


1.2
date	94.08.30.01.35.48;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.08.29.18.08.25;	author vandys;	state Exp;
branches;
next	;


desc
@Read/write handling (card-independent)
@


1.2
log
@Add support for coutning of dropped packets
@
text
@/*
 * rw.c
 *	Reads and writes to the NE2000
 */
#include <sys/fs.h>
#include <sys/assert.h>
#include <std.h>
#include <llist.h>
#include "ne.h"
#ifdef DEBUG
#include <stdio.h>	/* For printf */
#endif

#define ntohs(x) ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))

/*
 * Busy/waiter flags
 */
int tx_busy[NNE];		/* Tx Busy for each unit */
struct llist writers[NNE];	/* Who's waiting on each unit */
struct llist readers;		/* Who's waiting */
struct llist files;
ulong dropped;			/* # packets without reader */

/*
 * run_queue()
 *	If there's stuff in writers queue, launch next
 *
 * If there's nothing, clears "tx_busy"
 */
void
run_queue(int unit)
{
	struct file *f;
	struct msg m;

	/*
	 * Nobody waiting--adapter falls idle
	 */
	if (writers[unit].l_forw == &writers[unit]) {
		tx_busy[unit] = 0;
		return;
	}

	/*
	 * Remove next from list
	 */
	f = writers[unit].l_forw->l_data;
	ASSERT_DEBUG(f->f_io, "run_queue: on queue !f_io");
	ll_delete(f->f_io);
	f->f_io = 0;

	/*
	 * Launch I/O
	 */
	tx_busy[unit] = 1;
	ne_start(&adapters[unit], f);

	/*
	 * Success.
	 */
	m.m_arg = 0;
	m.m_nseg = 0;
	m.m_arg1 = 0;
	msg_reply(f->f_msg.m_sender, &m);
}

/*
 * rw_init()
 *	Initialize our queue data structures
 */
void
rw_init(void)
{
	int unit;

	for(unit = 0; unit < NNE; unit++) {
		ll_init(&writers[unit]);
	}
	ll_init(&readers);
}

/*
 * ne_write()
 *	Write to an open file
 */
void
ne_write(struct msg *m, struct file *f)
{
	struct attach *o = f->f_file;
	int unit = o->a_unit;

	/*
	 * Can only write to a true file, and only if open for writing.
	 */
	if (!o || !(f->f_perm & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Queue write, fail if we can't insert list element (VM
	 * exhausted?)
	 */
	ASSERT_DEBUG(f->f_io == 0, "ne_write: busy");
	f->f_io = ll_insert(&writers[unit], f);
	if (f->f_io == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}
	f->f_msg = *m;
	f->f_msg.m_arg = 0;

	if (!tx_busy[unit]) {
		run_queue(unit);
	}

	return;
}

/*
 * ne_read()
 *	Read bytes out of the current attachment or directory
 *
 * Directories get their own routine.
 */
void
ne_read(struct msg *m, struct file *f)
{
	struct attach *o;

	/*
	 * Directory--only one is the root
	 */
	if ((o = f->f_file) == 0) {
		ne_readdir(m, f);
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
	 * Queue as a reader
	 */
	ASSERT_DEBUG(f->f_io == 0, "ne_read: busy");
	f->f_io = ll_insert(&readers, f);
	if (f->f_io == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}
	f->f_msg = *m;
}

/*
 * ne_send_up()
 *	Send received packet to requestor(s)
 *
 * Given a buffer containing a received packet, walk the list of pending
 * readers and return packet if a matching type is found.  If none is
 * found, the packet is dropped.
 * len is length of packet data, excluding header, type and checksum.
 */
void
ne_send_up(char *buf, int len)
{
	struct llist *l, *ln;
	struct ether_header *eh;
	ushort etype;
	int sent;

	if ((len == 0) || (LL_EMPTY(&readers))) {
		return;
	}

	/*
	 * Walk pending reader list
	 */
	eh = (struct ether_header *)buf;
	etype = ntohs(eh->ether_type);
	sent = 0;
	for (l = LL_NEXT(&readers); l != &readers; l = ln) {
		struct file *f;
		ushort t2;

		/*
		 * Get next pointer now, before we might delete
		 */
		ln = LL_NEXT(l);

		/*
		 * Skip those who only have the directory itself
		 * open.
		 */
		f = l->l_data;
		ASSERT_DEBUG(f->f_file, "ne_send_up: dir");

		/*
		 * Give him the packet if he wants all (type 0) or
		 * has the right type open.
		 */
		t2 = f->f_file->a_type;
		if (!t2 || (t2 == etype)) {
			struct msg *m;

			m = &f->f_msg;
			if (m->m_nseg == 0) {
				/*
				 * He didn't provide a buffer, send
				 * him ours.
				 */
				m->m_nseg = 1;
				m->m_buf = buf;
				m->m_buflen = len;
			} else {
				/*
				 * Otherwise copy out our buffer to
				 * his (we're a DMA server) and go
				 * on.
				 */
				m->m_nseg = 0;
				bcopy(buf, m->m_buf, len);
			}
			m->m_arg = len;
			m->m_arg1 = 0;
			msg_reply(m->m_sender, m);
			ll_delete(l);
			ASSERT_DEBUG(f->f_io == l, "ne_send_up: mismatch");
			f->f_io = 0;
			sent += 1;
		}
	}

	/*
	 * Tally drops
	 */
	if (sent == 0) {
		dropped += 1;
	}
}
@


1.1
log
@Initial revision
@
text
@d23 1
a174 1
#ifdef DEBUG
a175 1
#endif
d201 1
a201 3
		if (f->f_file == 0) {
			continue;
		}
d238 4
a241 1
#ifdef DEBUG
d243 1
a243 1
		printf("No consumer for paktype 0x%x\n", etype);
a244 1
#endif
@
