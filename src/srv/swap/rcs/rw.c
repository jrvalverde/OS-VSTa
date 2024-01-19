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
date	93.11.16.02.48.52;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.01.18.47.55;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.26.23.29.53;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.12.38;	author vandys;	state Exp;
branches;
next	;


desc
@Read/write of the swap pseudo-device
@


1.4
log
@Source reorg
@
text
@/*
 * rw.c
 *	Routines for reading and writing swap
 *
 * Swap only support reads/writes of whole pages.
 */
#include <sys/param.h>
#include <sys/swap.h>
#include <std.h>

extern struct swapmap *swapent();

/*
 * swap_rw()
 *	Look up underlying swap device, forward operation
 */
void
swap_rw(struct msg *m, struct file *f, uint bytes)
{
	struct swapmap *s;
	uint blk;

	/*
	 * Check for permission, page alignment
	 */
	if (((m->m_op == FS_WRITE) || (m->m_op == FS_ABSWRITE)) &&
			!(f->f_perms & ACC_WRITE)) {
		msg_err(m->m_sender, EPERM);
		return;
	}
	if ((m->m_nseg != 1) || (bytes & (NBPG-1)) ||
			(f->f_pos & (NBPG-1))) {
		msg_err(m->m_sender, EINVAL);
		return;
	}
	blk = btop(f->f_pos);

	/*
	 * Find entry for next part of I/O
	 */
	if ((s = swapent(blk)) == 0) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Convert offset relative to beginning of this chunk of
	 * swap space.
	 */
	m->m_arg1 = ptob(blk - s->s_block + s->s_off);

	/*
	 * Send off the I/O
	 */
	if (msg_send(s->s_port, m) < 0) {
		msg_err(m->m_sender, strerror());
		return;
	}
	m->m_buflen = m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}
@


1.3
log
@Add more sanity checking.  Fix bug with treatment of units
in swap map.
@
text
@d8 1
a8 1
#include <swap/swap.h>
@


1.2
log
@Use official prototype and check nseg
@
text
@d21 1
d26 2
a27 1
	if ((m->m_op == FS_WRITE) && !(f->f_perms & ACC_WRITE)) {
d31 2
a32 1
	if ((m->m_nseg != 1) || (bytes & (NBPG-1))) {
d36 1
d41 1
a41 1
	if ((s = swapent(f->f_pos)) == 0) {
d45 6
@


1.1
log
@Initial revision
@
text
@d9 1
a10 1
extern char *strerror();
d29 1
a29 1
	if (bytes & (NBPG-1)) {
@
