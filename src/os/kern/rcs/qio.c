head	1.8;
access;
symbols
	V1_3_1:1.7
	V1_3:1.6
	V1_2:1.6
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.8
date	94.11.16.19.36.07;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.19.00.27.10;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.44.14;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.06.30.19.55.12;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.01.18.44.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.31.04.36.47;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.26.23.32.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.53.52;	author vandys;	state Exp;
branches;
next	;


desc
@QIO asynch I/O interface and support routines
@


1.8
log
@Tidy up semaphore count handling, add assertions.  Convert
atomic ops so routine matches data element size.
@
text
@/*
 * qio.c
 *	Routines for providing a asynch queued-I/O service
 */
#include <sys/param.h>
#include <sys/qio.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/msg.h>
#include <sys/seg.h>
#include <sys/vm.h>
#include <sys/fs.h>
#include <sys/port.h>
#include <alloc.h>

static sema_t qio_sema;		/* Counting semaphore for # elements */
static struct qio *qios = 0;	/* Free elements */
static sema_t qio_gate;		/* For waking a process to run the qio */
static struct qio		/* Elements waiting to be run */
	*qio_hd, *qio_tl;
static lock_t qio_lock;		/* Spinlock for run list */

static void free_qio(struct qio *);

/*
 * qio()
 *	Queue an I/O
 */
void
qio(struct qio *q)
{
	p_lock(&qio_lock, SPL0);
	q->q_next = 0;
	if (qio_hd == 0) {
		qio_hd = q;
	} else {
		qio_tl->q_next = q;
	}
	qio_tl = q;
	v_lock(&qio_lock, SPL0);
	v_sema(&qio_gate);
}

/*
 * qio_msg_send()
 *	Do a msg_send()'ish operation based on a QIO structure
 */
static void
qio_msg_send(struct qio *q)
{
	struct seg *s = 0;
	struct sysmsg *sm = 0;
	struct portref *pr = q->q_port;
	int error = 0;
	extern struct seg *kern_mem();

	/*
	 * Get our temp buffers
	 */
	ASSERT_DEBUG(q->q_cnt == NBPG, "qio: not a page");
	s = kern_mem(ptov(ptob(q->q_pp->pp_pfn)), NBPG);
	sm = MALLOC(sizeof(struct sysmsg), MT_SYSMSG);

	/*
	 * Become sole I/O through port
	 */
	p_sema(&pr->p_sema, PRIHI);
	p_lock(&pr->p_lock, SPL0);

	/*
	 * We lose if the server for the port has left
	 */
	if (!pr->p_port) {
		error = 1;
		goto out;
	}

	/*
	 * Send a seek+r/w
	 */
	sm->m_op = q->q_op;
	sm->m_arg = NBPG;
	sm->m_arg1 = q->q_off;
	sm->m_nseg = 1;
	sm->m_seg[0] = s;
	sm->m_sender = pr;
	pr->p_msg = sm;
	pr->p_state = PS_IOWAIT;
	ASSERT_DEBUG(sema_count(&pr->p_iowait) == 0, "qio_msg_send: p_iowait");
	queue_msg(pr->p_port, sm);
	p_sema_v_lock(&pr->p_iowait, PRIHI, &pr->p_lock);
	p_lock(&pr->p_lock, SPL0);
	if ((pr->p_port == 0) || (sm->m_arg < 0)) {
		error = 1;
		goto out;
	}

	/*
	 * We only support raw/DMA devices for swap, and rea-only
	 * for mapped files.
	 */
	ASSERT(sm->m_nseg == 0, "qio_msg_send: got back seg");

	/*
	 * Clean up and handle any iodone portion
	 */
out:
	v_lock(&pr->p_lock, SPL0);
	v_sema(&pr->p_svwait);
	v_sema(&pr->p_sema);
	if (sm) {
		FREE(sm, MT_SYSMSG);
	}

	/*
	 * If q_iodone isn't null, put result in q_cnt and call
	 * the function.
	 */
	if (q->q_iodone) {
		q->q_cnt = error;
		(*(q->q_iodone))(q);
	}

	/*
	 * Release qio element
	 */
	free_qio(q);
}

/*
 * run_qio()
 *	Syscall which the dedicated QIO threads remain within
 */
run_qio(void)
{
	struct qio *q;

	if (!isroot()) {
		return(-1);
	}

	/*
	 * Get a new qio structure, add to the pool
	 */
	q = MALLOC(sizeof(struct qio), MT_QIO);
	p_lock(&qio_lock, SPL0);
	q->q_next = qios;
	qios = q;
	v_lock(&qio_lock, SPL0);

	/*
	 * Bump the count allowed through the qio semaphore
	 */
	v_sema(&qio_sema);

	/*
	 * Endless loop
	 */
	for (;;) {
		/*
		 * Wait for work
		 */
		p_sema(&qio_gate, PRIHI);

		/*
		 * Extract one unit of work
		 */
		p_lock(&qio_lock, SPL0);
		q = qio_hd;
		ASSERT_DEBUG(q, "run_qio: gate mismatch with run");
		qio_hd = q->q_next;
#ifdef DEBUG
		if (qio_hd == 0) {
			qio_tl = 0;
		}
#endif
		v_lock(&qio_lock, SPL0);

		/*
		 * Do the work
		 */
		qio_msg_send(q);
	}
}

/*
 * alloc_qio()
 *	Allocate a queued I/O structure
 *
 * We throttle the number of queued operations with this routine.  It
 * may sleep, so it may not be called with locks held.
 */
struct qio *
alloc_qio(void)
{
	struct qio *q;

	p_sema(&qio_sema, PRIHI);
	p_lock(&qio_lock, SPL0);
	q = qios;
	ASSERT_DEBUG(q, "qio_alloc: bad sema count");
	qios = q->q_next;
	v_lock(&qio_lock, SPL0);
	return(q);
}

/*
 * free_qio()
 *	Free qio element back to pool when finished
 */
static void
free_qio(struct qio *q)
{
	p_lock(&qio_lock, SPL0);
	q->q_next = qios;
	qios = q;
	v_lock(&qio_lock, SPL0);
	v_sema(&qio_sema);
}

/*
 * init_qio()
 *	Called once to initialize the queued I/O system
 */
void
init_qio(void)
{
	init_sema(&qio_sema); set_sema(&qio_sema, 0);
	init_sema(&qio_gate); set_sema(&qio_gate, 0);
	qios = qio_hd = qio_tl = 0;
	init_lock(&qio_lock);
}
@


1.7
log
@Convert to MALLOC
@
text
@d89 1
a89 1
	set_sema(&pr->p_iowait, 0);
@


1.6
log
@Source reorg
@
text
@d62 1
a62 1
	sm = malloc(sizeof(struct sysmsg));
d112 1
a112 1
		free(sm);
d145 1
a145 1
	q = malloc(sizeof(struct qio));
@


1.5
log
@GCC warning cleanup
@
text
@d14 1
a14 1
#include <lib/alloc.h>
@


1.4
log
@Never assume value of p_iowait
@
text
@d23 2
@


1.3
log
@Add free_qio(), fix page I/O send to use seek+I/O interface,
fix message sending to conform to current practice (this code
was ancient!)
@
text
@a84 1
	queue_msg(pr->p_port, sm);
d87 2
@


1.2
log
@Change qio to support as many QIO threads as there
are calling threads to support
@
text
@d51 1
a51 1
	struct portref *pr;
d77 1
a77 1
	 * Send a seek.  XXX should we add read+seek and write+seek?
d79 6
a84 3
	sm->m_op = FS_SEEK;
	sm->m_arg = q->q_off;
	sm->m_nseg = 0;
d86 1
a93 14
	ASSERT_DEBUG(sm->m_nseg == 0, "qio: seek got msg seg");

	/*
	 * Send the block of memory
	 */
	sm->m_op = q->q_op;
	sm->m_arg = 0;
	sm->m_nseg = 1;
	sm->m_seg[0] = s;
	s = 0;			/* Freed by receiver */
	queue_msg(pr->p_port, sm);
	pr->p_state = PS_IOWAIT;
	p_sema_v_lock(&pr->p_iowait, PRIHI, &pr->p_lock);
	p_lock(&pr->p_lock, SPL0);
d96 2
a97 2
	 * XXX for non-raw-I/O we have received back a segment.
	 * Do we need to support this?
a98 4
	if ((pr->p_port == 0) || (sm->m_arg < 0)) {
		error = 1;
		goto out;
	}
d106 1
a110 3
	if (s) {
		free_seg(s);
	}
d120 5
d202 14
@


1.1
log
@Initial revision
@
text
@d14 1
a15 2
extern void *malloc();

d149 18
d168 3
d172 4
d186 4
d207 1
d211 1
d222 3
a224 13
	struct qio *q;
	int x;

	init_sema(&qio_sema);
	set_sema(&qio_sema, MAXQIO);
	for (x = 0, qios = 0; x < MAXQIO; ++x) {
		q = malloc(sizeof(struct qio));
		q->q_next = qios;
		qios = q;
	}
	init_sema(&qio_gate);
	set_sema(&qio_gate, MAXQIO);
	qio_hd = qio_tl = 0;
@
