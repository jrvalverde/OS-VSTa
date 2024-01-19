head	1.9;
access;
symbols
	V1_3_1:1.8
	V1_3:1.7
	V1_2:1.7
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.9
date	94.11.16.19.36.07;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.04.19.00.26.44;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.12.09.06.15.57;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.23.22.41.12;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.26.23.32.04;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.23.18.15.07;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.23.18.14.10;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.55.30;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of backing store
@


1.9
log
@Tidy up semaphore count handling, add assertions.  Convert
atomic ops so routine matches data element size.
@
text
@/*
 * vm_swap.c
 *	Routines and data structures for the swap pseudo-device
 *
 * swapdev must initially point to La-La land--we have to fake it until
 * the system comes up far enough to get some swap configured.  A tally
 * is run; the swap blocks are allocated starting at 1.  When a swap
 * manager shows up, the next swap request will have pending swap added
 * to the size.
 *
 * During DEBUG, swap frees are allowed and the space is leaked (lost).
 * During !DEBUG, we panic if we need to free swap before a manager has
 * been started.
 */
#include <sys/types.h>
#include <sys/port.h>
#include <sys/msg.h>
#include <sys/vm.h>
#include <sys/perm.h>
#include <sys/seg.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/swap.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/assert.h>
#include <alloc.h>

extern struct seg *kern_mem();
extern void freesegs();

struct portref *swapdev = 0;
sema_t swap_wait;
lock_t swap_lock;
static ulong swap_pending = 0L;	/* Pages consumed before swapper up */
ulong swap_leaked = 0L;		/* Pages lost before swap manager up */

/*
 * seg_physcopy()
 *	Copy out returned segments to a physical page
 */
static
seg_physcopy(struct sysmsg *sm, uint pfn)
{
	uint x, y, nbyte, cnt;
	char *p;
	struct seg *s;
	struct vas *vas = curthread->t_proc->p_vas;

	p = (char *)ptov(ptob(pfn));
	nbyte = NBPG;
	for (x = 0; nbyte && (x < sm->m_nseg); ++x) {
		s = sm->m_seg[x];

		/*
		 * Calculate amount to copy next
		 */
		if (s->s_len > nbyte) {
			cnt = nbyte;
		} else {
			cnt = s->s_len;
		}

		/*
		 * Attach the memory
		 */
		if (attach_seg(vas, s)) {
			return(1);
		}

		/*
		 * Copy the indicated amount
		 */
		y = copyin((char *)(s->s_pview.p_vaddr)+s->s_off, p, cnt);

		/*
		 * Detach and return error if copyin() failed
		 */
		detach_seg(s);
		if (y) {
			return(1);
		}

		/*
		 * Advance counters
		 */
		p += cnt;
		nbyte -= cnt;
	}
	return(0);
}

/*
 * pageio()
 *	Set up a synchronous page I/O
 *
 * This isn't used strictly for swap, though swapdev is probably
 * the most common destination for pageios.
 */
pageio(uint pfn, struct portref *pr, uint off, uint cnt, int op)
{
	struct sysmsg *sm;
	int error = 0;
	int holding_pr = 0, holding_port = 0;

	ASSERT_DEBUG((op == FS_ABSREAD) || (op == FS_ABSWRITE),
		"pageio: illegal op");

	ASSERT(pr, "pageio: null portref");
	/*
	 * Construct a system message
	 */
	sm = MALLOC(sizeof(struct sysmsg), MT_SYSMSG);
	sm->m_sender = pr;
	sm->m_op = op;
	sm->m_nseg = 1;
	sm->m_arg = cnt;
	sm->m_arg1 = off;
	sm->m_seg[0] = kern_mem(ptov(ptob(pfn)), cnt);

	/*
	 * One at a time through the portref
	 */
	p_sema(&pr->p_sema, PRIHI);
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
	ASSERT_DEBUG(sema_count(&pr->p_iowait) == 0, "pageio: p_iowait");
	pr->p_state = PS_IOWAIT;
	pr->p_msg = sm;

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
	 * If the server indicates error, set it and leave
	 */
	if (sm->m_arg == -1) {
		error = 1;
	} else {
		/*
		 * If we got segments back, copy them out and let
		 * them go.
		 */
		if (sm->m_nseg > 0) {
			error = seg_physcopy(sm, pfn);
			freesegs(sm);
		}
	}

	/*
	 * Let server go
	 */
	v_sema(&pr->p_svwait);
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
	v_sema(&pr->p_sema);
	return(error);
}

/*
 * set_swapdev()
 *	System call to set swap manager
 *
 * The calling process must have previously opened a portref to
 * the swap manager.  We verify permission and then steal the
 * portref and make it available as "swapdev".
 */
set_swapdev(port_t arg_port)
{
	struct portref *pr;
	extern struct portref *delete_portref();

	/*
	 * Make sure he's "root"
	 */
	if (!isroot()) {
		return(-1);
	}

	/*
	 * See if we already have one
	 */
	if (swapdev) {
		return(err(EBUSY));
	}

	/*
	 * Steal away his port
	 */
	pr = delete_portref(curthread->t_proc, arg_port);
	if (!pr) {
		return(-1);
	}

	/*
	 * It becomes swapdev
	 */
	ASSERT(pr, "set_swapdev: swap daemon died");
	swapdev = pr;
	v_sema(&swapdev->p_sema);
	return(0);
}

/*
 * alloc_swap()
 *	Request a chunk of swap from the swap manager
 */
ulong
alloc_swap(uint pages)
{
	long args[3];

	/*
	 * If there appears to be swap usage pending and the
	 * swap manager appears to be up, account for the
	 * space now.  This code assumes there are no other CPUs
	 * online yet, nor any real-time processes.
	 */
	args[0] = 0;
	if (swap_pending && swapdev) {
		(void)p_lock(&swap_lock, SPL0);
		if (swap_pending) {
			args[0] = swap_pending;
			swap_pending = 0;
		}
		v_lock(&swap_lock, SPL0);
	}

	/*
	 * We assume that the initial chunk of swap starts at 1--
	 * otherwise our bootup would have no way of assigning
	 * block numbers to the "pending" swap.
	 */
	if (args[0]) {
		ASSERT(alloc_swap(args[0]) == 1,
			"alloc_swap: pend != 1");
	}

	/*
	 * If no swap manager, run a "pending" tally
	 */
	if (!swapdev) {
		(void)p_lock(&swap_lock, SPL0);
		if (!swapdev) {
			args[0] = swap_pending+1;
			swap_pending += pages;
			v_lock(&swap_lock, SPL0);
			return(args[0]);
		}
		v_lock(&swap_lock, SPL0);
	}

	/*
	 * Loop until we get swap space
	 */
	for (;;) {
		ASSERT(swapdev, "alloc_swap: manager not ready");
		args[0] = pages; args[1] = 0;
		p_sema(&swapdev->p_sema, PRIHI);
		ASSERT(kernmsg_send(swapdev, SWAP_ALLOC, args) == 0,
			"alloc_swap: failed send");
		v_sema(&swapdev->p_sema);
		if (args[0] > 0) {
			return(args[0]);
		}
		p_sema(&swap_wait, PRIHI);
	}
}

/*
 * free_swap()
 *	Free back a chunk of swap to the swap manager
 */
void
free_swap(ulong block, uint pages)
{
	long args[3];

	/*
	 * This is usually the case of an exec() (discarding
	 * its old stack) during bootup.  We just leak the
	 * space and hope we'll have a swap manager soon.  The
	 * global is just to track how MUCH we leak.
	 */
	if (swapdev == 0) {
		swap_leaked += pages;
		return;
	}
	args[0] = block;
	args[1] = pages;
	p_sema(&swapdev->p_sema, PRIHI);
	ASSERT(kernmsg_send(swapdev, SWAP_FREE, args) == 0,
		"free_swap: send failed");
	v_sema(&swapdev->p_sema);
	if (blocked_sema(&swap_wait)) {
		vall_sema(&swap_wait);
	}
}

/*
 * init_swap()
 *	Initialize swapping code
 *
 * Note, this does not mean we're getting the swap server running.
 * This routine is just to initialize some basic mutexes.
 */
void
init_swap(void)
{
	init_sema(&swap_wait); set_sema(&swap_wait, 0);
	init_lock(&swap_lock);
}
@


1.8
log
@Convert to MALLOC
@
text
@d139 1
a139 1
	set_sema(&pr->p_iowait, 0);
@


1.7
log
@New third argument in args[]
@
text
@d26 1
d114 1
a114 1
	sm = malloc(sizeof(struct sysmsg));
d182 1
a182 1
		free(sm);
@


1.6
log
@Source reorg
@
text
@d238 1
a238 1
	long args[2];
d304 1
a304 1
	long args[2];
@


1.5
log
@Have to allow some swap leak during bootup until first swapdev
configured
@
text
@d24 1
a24 1
#include <swap/swap.h>
d27 1
a27 1
#include <lib/alloc.h>
@


1.4
log
@Fix calling parameters, tidy up comments
@
text
@d36 1
a305 1
#ifdef DEBUG
d307 4
a310 2
	 * During debugging it can be convenient to not bother running
	 * a swap manager, and just leak the swap space instead.
d313 1
a315 3
#else
	ASSERT(swapdev, "free_swap: manager dead");
#endif
@


1.3
log
@Get rid of debug printf's
@
text
@d5 9
a13 6
 * XXX swapdev must initially point to La-La land.  We'll have to
 * initially fake it until the system comes up far enough to get
 * some swap configured.  Perhaps just assume swap will be at least
 * 1 M, let people allocate, panic on pageout, and when we get our
 * first group of swap, insist it be >= 1 M, and "slip it in" as
 * the real space for the already-allocated swap blocks.
a37 5
 * The most privileged protection
 */
static struct perm kern_perm = {0};

/*
d216 1
a216 1
	pr = delete_portref(arg_port);
d242 2
a243 1
	 * space now.
@


1.2
log
@Extensive reworking of pageio(); now it works. :-)
Also a little diddling to be more forgiving about swap, at least
when we're debugging.
@
text
@a50 1
	printf("Copy %d segs to pfn 0x%x\n", sm->m_nseg, pfn);
a90 1
	printf(" ...copied\n"); dbg_enter();
a109 2
	printf("pageio pfn 0x%x portref 0x%x off 0x%x cnt 0x%x op %d\n",
		pfn, pr, off, cnt, op);
a131 1
		printf(" port gone\n");
a157 1
		printf(" error returned\n");
a159 2
		printf(" got back sm nseg %d arg %d\n",
			sm->m_nseg, sm->m_arg);
a164 1
			printf(" seg copy\n");
a174 1
	printf(" done\n");
@


1.1
log
@Initial revision
@
text
@d17 4
d24 1
d27 1
a27 1
extern void *malloc();
d40 57
d112 3
a114 1
	ASSERT(pr, "pageio: swap but no swapdev");
d122 2
a123 1
	sm->m_arg = off;
d136 1
d146 1
d163 1
d165 12
a176 1
		goto out;
a177 1
	ASSERT(sm->m_nseg == 0, "pageio: got segs back");
d179 4
d184 1
d316 9
d326 1
@
