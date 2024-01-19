head	1.14;
access;
symbols
	V1_3_1:1.13
	V1_3:1.11
	V1_2:1.11
	V1_1:1.10
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.14
date	94.11.16.19.36.07;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.04.19.20.28.13;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.04.19.00.27.43;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.12.09.06.16.36;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.11.16.02.44.14;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.11.12.20.28.04;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.10.03.00.26.17;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.09.18.17.51.12;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.23.22.41.50;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.13.17.12.44;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.26.23.32.46;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.19.15.34.31;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.09.17.09.17;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.51.36;	author vandys;	state Exp;
branches;
next	;


desc
@Message communication primitives
@


1.14
log
@Tidy up semaphore count handling, add assertions.  Convert
atomic ops so routine matches data element size.
@
text
@/*
 * msg.c
 *	Message handling routines
 *
 * These implement the primary user system service--message sending
 * and receiving.
 *
 * Message passing is done using memory mapping techniques.  Each
 * segment in a user message gets converted to a pview in terms of
 * the existing page set.  Because it's simply another view, it allows
 * process->process memory copying without the need for the kernel
 * to make an intermediate copy or fault in the pages.
 */
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/msg.h>
#include <sys/fs.h>
#include <sys/percpu.h>
#include <sys/port.h>
#include <sys/thread.h>
#include <sys/assert.h>
#include <sys/malloc.h>
#include <hash.h>

extern void free_seg(), ref_port(), deref_port();
extern struct portref *find_portref();
extern struct port *find_port();
extern struct seg *make_seg();
extern int attach_seg();

/*
 * lqueue_msg()
 *	Queue a message when port is already locked
 */
void
lqueue_msg(struct port *port, struct sysmsg *sm)
{
	sm->m_next = 0;
	if (port->p_tl) {
		port->p_tl->m_next = sm;
	}
	port->p_tl = sm;
	if (!port->p_hd) {
		port->p_hd = sm;
	}
	v_sema(&port->p_wait);
}

/*
 * queue_msg()
 *	Queue a message to the given port's queue
 *
 * This routine handles all locking of the given port.
 */
void
queue_msg(struct port *port, struct sysmsg *sm)
{
	spl_t s;

	/*
	 * Lock down destination port, put message in queue, and
	 * bump its sleeping semaphore.
	 */
	s = p_lock(&port->p_lock, SPLHI);
	lqueue_msg(port, sm);
	v_lock(&port->p_lock, s);
}

/*
 * freesegs()
 *	Release all references indicated by the segments of a sysmsg
 */
void
freesegs(struct sysmsg *sm)
{
	int x;

	for (x = 0; x < sm->m_nseg; ++x) {
		free_seg(sm->m_seg[x]);
	}
	sm->m_nseg = 0;
}

/*
 * unmapsegs()
 *	Unmap and free each segment for this segref
 *
 * Assumes caller has exclusive access to the segref
 */
void
unmapsegs(struct segref *segref)
{
	int x;
	struct seg *s;

	for (x = 0; x <= MSGSEGS; ++x) {
		if ((s = segref->s_refs[x]) == 0) {
			break;
		}
		detach_seg(s);
		free_seg(s);
		segref->s_refs[x] = 0;
	}
}

/*
 * mapsegs()
 *	Map each segment in a message into the user process
 *
 * Records the attachment under the segref structure, which
 * will be cleared of old mappings if necessary.
 */
static
mapsegs(struct proc *p, struct sysmsg *sm, struct segref *segref)
{
	int x;
	uint cnt = 0;
	struct seg *s;

	if (segref->s_refs[0]) {
		unmapsegs(segref);
	}
	for (x = 0; x < sm->m_nseg; ++x) {
		s = sm->m_seg[x];
		if (attach_seg(p->p_vas, s)) {
			int y;

			for (y = 0; y < x; ++y) {
				s = sm->m_seg[y];
				segref->s_refs[y] = 0;
				detach_seg(s);
			}
			err(ENOMEM);
			return(-1);
		}
		segref->s_refs[x] = s;
		cnt += s->s_len;
	}
	segref->s_refs[sm->m_nseg] = 0;
	return(cnt);
}

/*
 * sm_to_m()
 *	Convert the segment information from sysmsg to struct msg format
 */
static void
sm_to_m(struct sysmsg *sm, struct msg *m)
{
	int x;
	seg_t *s;
	struct seg *seg;

	m->m_sender = (port_t)sm->m_sender;
	m->m_op = sm->m_op & ~M_READ;
	m->m_arg = sm->m_arg;
	m->m_arg1 = sm->m_arg1;
	m->m_nseg = sm->m_nseg;
	for (x = 0, s = m->m_seg; x < sm->m_nseg; ++x,++s) {
		seg = sm->m_seg[x];
		s->s_buf = (char *)(seg->s_pview.p_vaddr) + seg->s_off;
		s->s_buflen = seg->s_len;
	}
	sm->m_nseg = 0;
}

/*
 * m_to_sm()
 *	Convert from user segments to sysmsg format
 *
 * For !M_READ, creates segments for each chunk of user memory,
 * walks across all the "segments" of the user's message,
 * converts to internal format, and places under the sysmsg.
 *
 * For M_READ, only the header part is copied.  The user buffers
 * will be filled out once an answer comes back from the server.
 *
 * Can sleep in make_seg().
 *
 * On error, sets err() and returns -1.  On success, returns 0.
 */
static
m_to_sm(struct vas *vas, struct msg *m, struct sysmsg *sm)
{
	int x;
	struct seg *seg;
	seg_t *s;

	/*
	 * Sanity check # segments
	 */
	if (m->m_nseg > MSGSEGS) {
		return(err(EINVAL));
	}

	/*
	 * Copy over header part
	 */
	sm->m_op = m->m_op;
	sm->m_arg = m->m_arg;
	sm->m_arg1 = m->m_arg1;

	/*
	 * Walk user segments, construct struct seg's for each part
	 */
	if (!(sm->m_op & M_READ)) {
		for (x = 0, s = m->m_seg; x < m->m_nseg; ++x, ++s) {
			/*
			 * On error, have to go back and clean up the
			 * segments we've already constructed.  Then
			 * return error.
			 */
			seg = make_seg(vas, s->s_buf, s->s_buflen);
			if (seg == 0) {
				int y;

				for (y = 0; y < x; ++y) {
					free_seg(sm->m_seg[y]);
					sm->m_seg[y] = 0;
				}
				sm->m_nseg = 0;
				return(err(EFAULT));
			}
			sm->m_seg[x] = seg;
		}
		sm->m_nseg = m->m_nseg;
	} else {
		sm->m_nseg = 0;
	}
	return(0);
}

/*
 * msg_send()
 *	Send a message to a port
 *
 * msg_send() does the job of validating and packing up the
 * message, and queueing it to the appropriate destination.
 * The receiving side maps it into the process address space.
 * When the receiving side msg_reply()'s, this routine picks up
 * any corresponding reply segments and maps them back into
 * this process' address space.
 */
msg_send(port_t arg_port, struct msg *arg_msg)
{
	struct portref *pr;
	int holding_pr = 0;
	struct msg *m;
	struct sysmsg *sm = 0;
	struct proc *p = curthread->t_proc;
	int error = 0;

	/*
	 * Get message body
	 */
	m = MALLOC(sizeof(struct msg), MT_MSG);
	if (copyin(arg_msg, m, sizeof(struct msg))) {
		error = err(EFAULT);
		goto out;
	}

	/*
	 * Protect our reserved messages types.  M_TIME is a special
	 * case because I used to think I'd generate these from the
	 * kernel, but currently I do it through a user-space thread.
	 * I could renumber it out of the reserved range--TBD.
	 *
	 * We mask the low bits to avoid being confused by the
	 * M_READ bit in the higher bits.
	 */
	if ((m->m_op & 0xFFF) < M_RESVD) {
		if ((m->m_op & 0xFFF) != M_TIME) {
			FREE(m, MT_MSG);
			return(err(EINVAL));
		}
	}

	/*
	 * Construct a system message
	 */
	sm = MALLOC(sizeof(struct sysmsg), MT_SYSMSG);
	if (m_to_sm(p->p_vas, m, sm)) {
		error = -1;
		goto out;
	}

	/*
	 * Validate port ID.  On successful non-poisoned port, the
	 * semaphore is held for our handle on the port.
	 */
	pr = find_portref(p, arg_port);
	if (pr == 0) {
		/* find_portref() sets err() */
		error = -1;
		goto out;
	}
	holding_pr = 1;
	sm->m_sender = pr;
	pr->p_msg = sm;

	/*
	 * Set up our message transfer state
	 */
	ASSERT_DEBUG(sema_count(&pr->p_iowait) == 0, "msg_send: p_iowait");
	pr->p_state = PS_IOWAIT;

	/*
	 * Put message on queue
	 */
	queue_msg(pr->p_port, sm);

	/*
	 * Now wait for the I/O to finish or be interrupted
	 */
	if (p_sema_v_lock(&pr->p_iowait, PRICATCH, &pr->p_lock)) {
		struct sysmsg *sm2;

		/*
		 * Oops.  Interrupted.  Grapple with the server for
		 * control of the in-progress message.  Allocate a
		 * message before we take the lock, in case we need
		 * it.
		 */
		sm2 = MALLOC(sizeof(struct sysmsg), MT_SYSMSG);
		p_lock(&pr->p_lock, SPL0);

		/*
		 * Based on the state, either abort the I/O or
		 * accept (and ignore) the completion.
		 */
		switch (pr->p_state) {
		case PS_IOWAIT: {
			/*
			 * Send an M_ABORT and then wait for completion
			 * ignoring further interrupts.
			 */
			sm2->m_sender = pr;
			sm2->m_op = M_ABORT;
			sm2->m_arg = sm2->m_arg1 = 0;
			sm2->m_nseg = 0;
			queue_msg(pr->p_port, sm2);
			pr->p_state = PS_ABWAIT;
			p_sema_v_lock(&pr->p_iowait, PRIHI, &pr->p_lock);
			ASSERT_DEBUG(pr->p_state == PS_ABDONE,
				"msg_send: bad abdone");
			break;
		}

		case PS_IODONE:
			/*
			 * We raced with server completion.  The transaction
			 * is complete, but we still return an error.  The
			 * server already V'ed our iowait sema, so clear
			 * it for next time.
			 */
			set_sema(&pr->p_iowait, 0);
			v_lock(&pr->p_lock, SPL0);
			break;
		default:
			printf("Port 0x%x, state %d\n", pr, pr->p_state);
			ASSERT(0, "msg_send: illegal PS state");
		}
		FREE(sm2, MT_SYSMSG);

		/*
		 * Done with I/O on port.  Return error.
		 */
		error = err(EINTR);
		goto out;
	}

	/*
	 * If the server indicates error, set it and leave
	 */
	if (sm->m_arg == -1) {
		error = err(sm->m_err);
		goto out;
	}

	if (sm->m_nseg) {
		struct segref segrefs;

		/*
		 * Attach the returned segments to the user address
		 * space.
		 */
		segrefs.s_refs[0] = 0;
		error = mapsegs(p, sm, &segrefs);
		if (error == -1) {
			goto out;
		}

		/*
		 * Copy out segments to user's buffer, then let them go
		 */
		error = copyoutsegs(sm, m);
		unmapsegs(&segrefs);
		if (error == -1) {
			goto out;
		}

		/*
		 * Translate rest of message to user format, give back
		 * to user.
		 */
		sm_to_m(sm, m);
		if (copyout(arg_msg, m, sizeof(struct msg))) {
			error = err(EFAULT);
		}
	}

	/*
	 * If return value is not otherwise indicated, use m_arg
	 */
	if (error == 0) {
		error = sm->m_arg;
	}

out:
	/*
	 * Clean up and return success/failure
	 */
	if (holding_pr) {
		v_sema(&pr->p_svwait);
		v_sema(&pr->p_sema);
	}
	if (m) {
		FREE(m, MT_MSG);
	}
	if (sm) {
		if (sm->m_nseg) {
			freesegs(sm);
		}
		FREE(sm, MT_SYSMSG);
	}
	return(error);
}

/*
 * new_client()
 *	Record a new client for this server port
 */
static void
new_client(struct portref *pr)
{
	struct proc *p = curthread->t_proc;

	p_sema(&p->p_sema, PRIHI);
	ASSERT_DEBUG(!hash_lookup(p->p_prefs, (long)pr),
		"new_client: already hashed");
	hash_insert(p->p_prefs, (long)pr, pr);
	v_sema(&p->p_sema);
}

/*
 * del_client()
 *	Delete an existing client from our hash
 */
static void
del_client(struct portref *pr)
{
	struct proc *p = curthread->t_proc;

	p_sema(&p->p_sema, PRIHI);
	ASSERT_DEBUG(hash_lookup(p->p_prefs, (long)pr),
		"del_client: can't find");
	hash_delete(p->p_prefs, (long)pr);
	v_sema(&p->p_sema);
}

/*
 * receive_isr()
 *	Copy out an M_ISR message to the user
 */
static
receive_isr(int isr, int nintr, struct msg *arg_msg)
{
	struct msg m;

	/*
	 * Build a user message
	 */
	m.m_op = M_ISR;
	m.m_arg = isr;
	m.m_arg1 = nintr;
	m.m_nseg = 0;

	/*
	 * Send it out to him
	 */
	return(copyout(arg_msg, &m, sizeof(m)));
}

/*
 * msg_receive()
 *	Receive next message from queue
 */
msg_receive(port_t arg_port, struct msg *arg_msg)
{
	struct port *port;
	int holding_port = 0;
	struct proc *p = curthread->t_proc;
	struct sysmsg *sm = 0;
	struct msg *m = 0;
	int error = 0;
	struct portref *pr;

	/*
	 * Look up port, become sole process doing I/O through it
	 */
	port = find_port(p, arg_port);
	if (!port) {
		return(-1);
	}
	holding_port = 1;

	/*
	 * Wait for something to arrive for us
	 */
	error = p_sema_v_lock(&port->p_wait, PRICATCH, &port->p_lock);
	p_lock(&port->p_lock, SPLHI);

	/*
	 * Interrupted system call.
	 */
	if (error) {
		error = err(EINTR);
		goto out;
	}
	ASSERT_DEBUG(port->p_hd,
		"msg_receive: p_wait/p_hd disagree");

	/*
	 * Extract next message, then release port
	 */
	sm = port->p_hd;
	port->p_hd = sm->m_next;
	if (port->p_hd == 0) {
		port->p_tl = 0;
	}

	/*
	 * With lock held, at SPLHI, check for M_ISR.  These are
	 * special messages we handle carefully to avoid losing
	 * an interrupt.
	 */
	if (sm->m_op == M_ISR) {
		int isr, nintr;

		/*
		 * Record ISR and count before we release the sysmsg
		 * back for further use.
		 */
		isr = sm->m_arg;
		nintr = sm->m_arg1;

		/*
		 * Flag sysmsg as "off the queue"
		 */
		sm->m_op = 0;
		v_lock(&port->p_lock, SPL0); holding_port = 0;

		/*
		 * Make sure we don't fiddle with the sysmsg any more.
		 * Call our function to pass out an ISR notification.
		 */
		sm = 0;
		error = receive_isr(isr, nintr, arg_msg);
		goto out;
	}

	/*
	 * Have our message, release port.
	 */
	pr = sm->m_sender;

	/*
	 * Connect messages are special; the buffer is the array
	 * of permissions which the connector possesses.  The portref
	 * is new and must be added to our hash.
	 */
	if (sm->m_op == M_CONNECT) {
		extern struct seg *kern_mem();

		ref_port(port, pr);
		v_lock(&port->p_lock, SPL0); holding_port = 0;
		new_client(pr);

		sm->m_seg[0] = kern_mem(sm->m_seg[0], sm->m_nseg);
		sm->m_nseg = 1;

	/*
	 * Disconnect message mean we remove the portref from
	 * our hash, tell our server he's gone, and let the
	 * client complete his disconnect and free his portref.
	 */
	} else if (sm->m_op == M_DISCONNECT) {
		ASSERT_DEBUG(pr->p_state == PS_CLOSING,
			"msg_receive: DISC but not CLOSING");
		ASSERT_DEBUG(pr->p_port == port,
			"msg_receive: DISC pr mismatch");
		deref_port(port, pr);
		v_lock(&port->p_lock, SPL0); holding_port = 0;
		del_client(pr);
		unmapsegs(&pr->p_segs);
		v_sema(&pr->p_iowait);
	} else {
		v_lock(&port->p_lock, SPL0); holding_port = 0;
	}

	/*
	 * We now have a message, and are running under an address space
	 * into which we now may want to map the parts of the message.
	 */
	m = MALLOC(sizeof(struct msg), MT_MSG);
	if (sm->m_nseg) {
		error = mapsegs(p, sm, &pr->p_segs);
		if (error == -1) {
			goto out;
		}
	}
	sm_to_m(sm, m);
	if (copyout(arg_msg, m, sizeof(struct msg))) {
		error = err(EFAULT);
		goto out;
	}

	/*
	 * All done.  Release any remaining locks and return
	 * success/failure.
	 */
out:
	if (holding_port) {
		v_lock(&port->p_lock, SPL0);
	}
	v_sema(&port->p_sema);
	if (m) {
		FREE(m, MT_MSG);
	}
	if (sm && sm->m_nseg) {
		freesegs(sm);
		sm->m_nseg = 0;
	}
	return(error);
}

/*
 * msg_reply()
 *	Reply to a message received through msg_receive()
 */
msg_reply(long arg_who, struct msg *arg_msg)
{
	struct proc *p = curthread->t_proc;
	struct portref *pr;
	int holding_pr = 0;
	struct msg *m;
	struct sysmsg *sm;
	int error = 0;

	/*
	 * Get a copy of the user's reply message
	 */
	m = MALLOC(sizeof(struct msg), MT_MSG);
	if (copyin(arg_msg, m, sizeof(struct msg))) {
		FREE(m, MT_MSG);
		return(err(EFAULT));
	}
	sm = MALLOC(sizeof(struct sysmsg), MT_SYSMSG);

	/*
	 * Try to map segments into sysmsg format
	 */
	if (m_to_sm(p->p_vas, m, sm)) {
		error = -1;
		goto out;
	}

	/*
	 * Lock down proc and try to map arg_who onto a known
	 * connection.  Ironically, arg_who *is* the address of
	 * the portref, but we can't trust it until we've found
	 * it in our hash table.
	 */
	p_sema(&p->p_sema, PRIHI);
	pr = hash_lookup(p->p_prefs, arg_who);
	if (pr) {
		unmapsegs(&pr->p_segs);
		p_lock(&pr->p_lock, SPL0); holding_pr = 1;
	}
	v_sema(&p->p_sema);

	/*
	 * If we didn't find the portref, bounce them.
	 */
	if (!pr) {
		error = err(EINVAL);
		goto out;
	}

	/*
	 * We now have the portref locked, and the reply message
	 * in hand.  Take action based on the state of the portref
	 * and the type of message we're delivering.
	 */
	switch (pr->p_state) {
	/*
	 * The usual case--he's waiting for our message.  Give it
	 * to him, and wake him up.  We wait for him to copy out
	 * the message, and let us free.
	 */
	case PS_IOWAIT:
		if ((pr->p_msg->m_op == M_DUP) && (sm->m_arg != -1)) {
			struct port *port = pr->p_port;
			struct portref *newpr = (struct portref *)
				(pr->p_msg->m_arg);

			ASSERT_DEBUG(newpr->p_port == port,
				"msg_reply: newpr != port");
			p_lock(&port->p_lock, SPLHI);
			ref_port(pr->p_port, newpr);
			v_lock(&port->p_lock, SPL0);
			v_lock(&pr->p_lock, SPL0); holding_pr = 0;
			v_sema(&pr->p_iowait);
			new_client(newpr);
		} else {
			/*
			 * Give him the sysmsg.
			 */
			*(pr->p_msg) = *sm;	
			sm->m_nseg = 0;		/* He has them */

			/*
			 * Let him run
			 */
			pr->p_state = PS_IODONE;
			set_sema(&pr->p_svwait, 0);
			v_sema(&pr->p_iowait);
			p_sema_v_lock(&pr->p_svwait, PRIHI, &pr->p_lock);
			holding_pr = 0;
		}
		break;

	/*
	 * He's requested an abort.  Flag confirmation if this is
	 * an M_ABORT message response, otherwise discard this
	 * message so he can continue to wait for the M_ABORT
	 * response.
	 */
	case PS_ABWAIT:
		if (sm->m_op != M_ABORT) {
			error = err(EIO);
			goto out;
		}
		pr->p_state = PS_ABDONE;
		v_sema(&pr->p_iowait);
		break;

	/*
	 * The server's hosed.  It would appear he's msg_reply()'ed to
	 * a client who's already finished.
	 */
	case PS_IODONE:
	case PS_ABDONE:
	default:
		error = err(EINVAL);
		break;
	}

	/*
	 * All done.  Release locks, free memory, return result
	 */
out:
	if (holding_pr) {
		v_lock(&pr->p_lock, SPL0);
	}
	if (m) {
		FREE(m, MT_MSG);
	}
	if (sm) {
		if (sm->m_nseg) {
			freesegs(sm);
		}
		FREE(sm, MT_SYSMSG);
	}
	return(error);
}
@


1.13
log
@Back out needless swap--no segs left in old sysmsg, so save
the cycles
@
text
@d304 1
a304 1
	set_sema(&pr->p_iowait, 0);
d352 3
a354 1
			 * is complete, but we still return an error.
d356 1
d424 1
a424 3
		if (blocked_sema(&pr->p_svwait)) {
			v_sema(&pr->p_svwait);
		}
@


1.12
log
@Convert to MALLOC.  Clean up old MSGTRACE stuff.  Swap sysmsg's
on msg_reply(), don't just overwrite the user's.
@
text
@a724 2
			struct sysmsg smtmp;

d726 1
a726 2
			 * Take his segments (if any) and give him
			 * ours.  We'll clean his up below.
a727 1
			smtmp = *(pr->p_msg);
d729 1
a729 1
			*sm = smtmp;
@


1.11
log
@Add ptrace hooks
@
text
@d22 1
d25 1
a25 3
/* #define MSGTRACE /* */

extern void *malloc(), free(), free_seg(), ref_port(), deref_port();
d256 1
a256 1
	m = malloc(sizeof(struct msg));
d273 1
a273 1
			free(m);
a276 4
#ifdef MSGTRACE
	printf("msg_send port 0x%x op 0x%x segs %d\n",
		arg_port, m->m_op, m->m_nseg);
#endif
d281 1
a281 1
	sm = malloc(sizeof(struct sysmsg));
d324 1
a324 1
		sm2 = malloc(sizeof(struct sysmsg));
d360 1
a360 1
		free(sm2);
a375 3
#ifdef MSGTRACE
	printf("  ...send receives nseg %d\n", sm->m_nseg);
#endif
d427 1
a427 1
		free(m);
d433 1
a433 1
		free(sm);
d540 1
a540 4
#ifdef MSGTRACE
	printf("msg_receive port 0x%x op 0x%x segs %d\n",
		arg_port, sm->m_op, sm->m_nseg);
#endif
d614 1
a614 1
	m = malloc(sizeof(struct msg));
d637 1
a637 1
		free(m);
d662 1
a662 1
	m = malloc(sizeof(struct msg));
d664 1
a664 1
		free(m);
d667 1
a667 5
#ifdef MSGTRACE
	printf("msg_reply port 0x%x op 0x%x segs %d\n",
		arg_who, m->m_op, m->m_nseg);
#endif
	sm = malloc(sizeof(struct sysmsg));
d725 13
a737 2
			*(pr->p_msg) = *sm;
			sm->m_nseg = 0;		/* He has them now */
d780 1
a780 1
		free(m);
d786 1
a786 1
		free(sm);
@


1.10
log
@Source reorg
@
text
@d14 2
a16 1
#include <sys/types.h>
a20 1
#include <sys/proc.h>
@


1.9
log
@Don't be so lazy in unmapping segments--they can sit around forever
for the case of cached executables.
@
text
@d22 1
a22 1
#include <lib/hash.h>
@


1.8
log
@Fix comparison w. MSGSEGS.  Also simplify another
comparison, without changing its sense.
@
text
@d701 1
@


1.7
log
@Return EFAULT on failed mapin, since this is probably a better
description of the error.
@
text
@d97 1
a97 1
	for (x = 0; x < MSGSEGS+1; ++x) {
d193 1
a193 1
	if (m->m_nseg >= MSGSEGS) {
@


1.6
log
@Have to null tail pointer; used by linked list routines
@
text
@d223 1
a223 1
				return(err(EINVAL));
@


1.5
log
@Add assertion of portref being from right port
@
text
@a544 1
#ifdef DEBUG
a547 1
#endif
@


1.4
log
@Add firewall for bogus message types
@
text
@d612 2
@


1.3
log
@Abort message needs to be flagged as having no segments
@
text
@d262 16
@


1.2
log
@Don't hold lock while doing p_sema in new_client
@
text
@d329 1
@


1.1
log
@Initial revision
@
text
@d718 2
d724 5
a729 5
		pr->p_state = PS_IODONE;
		set_sema(&pr->p_svwait, 0);
		v_sema(&pr->p_iowait);
		p_sema_v_lock(&pr->p_svwait, PRIHI, &pr->p_lock);
		holding_pr = 0;
@
