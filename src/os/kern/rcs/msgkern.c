head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.3
	V1_2:1.3
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.5
date	94.11.16.19.36.07;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.19.00.26.44;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.12.09.06.15.57;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.52.06;	author vandys;	state Exp;
branches;
next	;


desc
@Kernel interface for sending messages
@


1.5
log
@Tidy up semaphore count handling, add assertions.  Convert
atomic ops so routine matches data element size.
@
text
@/*
 * msgkern.c
 *	Routines to support message passing within the kernel
 */
#include <sys/msg.h>
#include <sys/port.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/assert.h>

/*
 * kernmsg_send()
 *	Send a message to a port, within the kernel
 *
 * Only useful for messages without segments.  Assumes caller has
 * already interlocked on the use of the portref.
 *
 * args[] receives m_arg, m_arg1, and m_op.  It must have room
 * for these 3 longs.
 */
kernmsg_send(struct portref *pr, int op, long *args)
{
	struct sysmsg *sm;
	int holding_pr = 0, error = 0;

	/*
	 * Construct a system message
	 */
	sm = MALLOC(sizeof(struct sysmsg), MT_SYSMSG);
	sm->m_sender = pr;
	sm->m_op = op;
	sm->m_nseg = 0;
	sm->m_arg = args[0];
	sm->m_arg1 = args[1];
	pr->p_msg = sm;

	/*
	 * Interlock with server
	 */
	p_lock(&pr->p_lock, SPL0); holding_pr = 1;

	/*
	 * If port gone, I/O error
	 */
	if (pr->p_port == 0) {
		error = 1;
		goto out;
	}

	/*
	 * Set up our message transfer state
	 */
	ASSERT_DEBUG(sema_count(&pr->p_iowait) == 0, "kernmsg_send: p_iowait");
	pr->p_state = PS_IOWAIT;

	/*
	 * Put message on queue
	 */
	queue_msg(pr->p_port, sm);

	/*
	 * Now wait for the I/O to finish or be interrupted
	 */
	p_sema_v_lock(&pr->p_iowait, PRIHI, &pr->p_lock);
	holding_pr = 0;

	/*
	 * Release the server
	 */
	v_sema(&pr->p_svwait);

	/*
	 * If the server indicates error, set it and leave
	 */
	if (sm->m_arg == -1) {
		error = 1;
		goto out;
	}
	ASSERT(sm->m_nseg == 0, "kernmsg_send: got segs back");
	args[0] = sm->m_arg;
	args[1] = sm->m_arg1;
	args[2] = sm->m_op;

out:
	/*
	 * Clean up and return success/failure
	 */
	if (holding_pr) {
		v_lock(&pr->p_lock, SPL0);
	}
	if (sm) {
		FREE(sm, MT_SYSMSG);
	}
	return(error);
}
@


1.4
log
@Convert to MALLOC
@
text
@d53 1
a53 1
	set_sema(&pr->p_iowait, 0);
@


1.3
log
@New third argument in args[]
@
text
@d8 1
a8 1
#include <alloc.h>
d29 1
a29 1
	sm = malloc(sizeof(struct sysmsg));
d92 1
a92 1
		free(sm);
@


1.2
log
@Source reorg
@
text
@d17 3
d82 1
@


1.1
log
@Initial revision
@
text
@d8 1
a8 1
#include <lib/alloc.h>
@
