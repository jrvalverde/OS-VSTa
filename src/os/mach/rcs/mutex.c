head	1.10;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5
	V1_1:1.5
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.10
date	94.12.23.05.12.47;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.12.21.05.27.06;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.11.16.19.34.56;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.08.30.19.48.40;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.04.02.01.11;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.31.04.40.52;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.30.01.11.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.19.15.36.32;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.09.17.11.47;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.06.12;	author vandys;	state Exp;
branches;
next	;


desc
@i386/ISA uniprocessor implementation of mutex locks and semaphores
@


1.10
log
@Use .h def
@
text
@/*
 * mutex.c
 *	Uniprocessor i386 implementation of mutual exclusion
 *
 * Mutual exclusion's not very hard on a uniprocessor, eh?
 */
#include <sys/mutex.h>
#include <sys/assert.h>
#include <sys/thread.h>
#include <sys/percpu.h>
#include <sys/sched.h>

extern lock_t runq_lock;

/*
 * p_lock()
 *	Take spinlock
 */
spl_t
p_lock(lock_t *l, spl_t s)
{
	int x;

	if (s == SPLHI) {
		x = cli();
	} else {
		x = 1;
	}
	ASSERT_DEBUG(l->l_lock == 0, "p_lock: deadlock");
	l->l_lock = 1;
	ATOMIC_INCW(&cpu.pc_locks);
	return(x ? SPL0 : SPLHI);
}

/*
 * cp_lock()
 *	Conditional take of spinlock
 */
spl_t
cp_lock(lock_t *l, spl_t s)
{
	int x;

	if (s == SPLHI) {
		x = cli();
	} else {
		x = 1;
	}
	if (l->l_lock) {
		if ((s == SPLHI) && x) {
			sti();
		}
		return(-1);
	}
	l->l_lock = 1;
	ATOMIC_INCW(&cpu.pc_locks);
	return(x ? SPL0 : SPLHI);
}

/*
 * v_lock()
 *	Release spinlock
 */
void
v_lock(lock_t *l, spl_t s)
{
	ASSERT_DEBUG(l->l_lock, "v_lock: not held");
	l->l_lock = 0;
	if (s == SPL0) {
		sti();
	}
	ATOMIC_DECW(&cpu.pc_locks);
}

/*
 * init_lock()
 *	Initialize lock to "not held"
 */
void
init_lock(lock_t *l)
{
	l->l_lock = 0;
}

/*
 * q_sema()
 *	Queue a thread under a semaphore
 *
 * Assumes semaphore is locked.
 */
static void
q_sema(struct sema *s, struct thread *t)
{
	if (!s->s_sleepq) {
		s->s_sleepq = t;
		t->t_hd = t->t_tl = t;
	} else {
		struct thread *t2;

		t2 = s->s_sleepq;
		t->t_hd = t2;
		t->t_tl = t2->t_tl;
		t->t_tl->t_hd = t;
		t2->t_tl = t;
	}
}

/*
 * dq_sema()
 *	Remove a thread from a semaphore sleep list
 *
 * Assumes semaphore is locked.
 */
void
dq_sema(struct sema *s, struct thread *t)
{
	ASSERT_DEBUG(s->s_count < 0, "dq_sema: bad count");
	s->s_count += 1;
	if (t->t_hd == t) {
		s->s_sleepq = 0;
	} else {
		t->t_hd->t_tl = t->t_tl;
		t->t_tl->t_hd = t->t_hd;
		if (s->s_sleepq == t) {
			s->s_sleepq = t->t_hd;
		}
	}
#ifdef DEBUG
	t->t_hd = t->t_tl = 0;
#endif
}

/*
 * p_sema()
 *	Take semaphore, sleep if can't
 */
int
p_sema(sema_t *s, pri_t p)
{
	struct thread *t;

	ASSERT_DEBUG(cpu.pc_locks == 0, "p_sema: locks held");
	ASSERT_DEBUG(s->s_lock.l_lock == 0, "p_sema: deadlock");

	/*
	 * Counts > 0, just decrement the count and go
	 */
	(void)p_lock(&s->s_lock, SPLHI);
	s->s_count -= 1;
	if (s->s_count >= 0) {
		v_lock(&s->s_lock, SPL0);
		return(0);
	}

	/*
	 * We're going to sleep.  Add us at the tail of the
	 * queue, and relinquish the processor.
	 */
	t = curthread;
	t->t_wchan = s;
	t->t_nointr = (p == PRIHI);
	t->t_intr = 0;	/* XXX this can race with notify */
	q_sema(s, t);
	p_lock(&runq_lock, SPLHI);
	v_lock(&s->s_lock, SPLHI);
	t->t_state = TS_SLEEP;
	ATOMIC_DEC(&num_run);
	swtch();

	/*
	 * We're back.  If we have an event pending, give up on the
	 * semaphore and return the right result.
	 */
	t->t_nointr = 0;
	if (t->t_intr) {
		ASSERT_DEBUG(t->t_wchan == 0, "p_sema: intr w. wchan");
		ASSERT_DEBUG(p != PRIHI, "p_sema: intr w. PRIHI");
		if (p != PRICATCH) {
			longjmp(t->t_qsav, 1);
		}
		return(1);
	}

	/*
	 * We were woken up to be next with the semaphore.  Our waker
	 * did the reference count update, so we just return.
	 */
	return(0);
}

/*
 * cp_sema()
 *	Conditional p_sema()
 */
int
cp_sema(sema_t *s)
{
	spl_t spl;
	int x;

	ASSERT_DEBUG(s->s_lock.l_lock == 0, "p_sema: deadlock");

	spl = p_lock(&s->s_lock, SPLHI);
	if (s->s_count > 0) {
		s->s_count -= 1;
		x = 0;
	} else {
		x = -1;
	}
	v_lock(&s->s_lock, spl);
	return(x);
}

/*
 * v_sema()
 *	Release a semaphore
 */
void
v_sema(sema_t *s)
{
	struct thread *t;
	spl_t spl;

	spl = p_lock(&s->s_lock, SPLHI);
	if (s->s_sleepq) {
		ASSERT_DEBUG(s->s_count < 0, "v_sema: too many sleepers");
		t = s->s_sleepq;
		ASSERT_DEBUG(t->t_wchan == s, "v_sema: mismatch");
		dq_sema(s, t);
		t->t_wchan = 0;
		setrun(t);
	} else {
		/* dq_sema() does it otherwise */
		s->s_count += 1;
	}
	v_lock(&s->s_lock, spl);
}

/*
 * vall_sema()
 *	Kick everyone loose who's sleeping on the semaphore
 */
void
vall_sema(sema_t *s)
{
	while (s->s_count < 0) {
		/* XXX races on MP; have to expand v_sema here */
		v_sema(s);
	}
}

/*
 * blocked_sema()
 *	Tell if anyone's sleeping on the semaphore
 */
int
blocked_sema(sema_t *s)
{
	return (s->s_count < 0);
}

/*
 * init_sema()
 * 	Initialize semaphore
 *
 * The s_count starts at 1.
 */
void
init_sema(sema_t *s)
{
	s->s_count = 1;
	s->s_sleepq = 0;
	init_lock(&s->s_lock);
}

/*
 * set_sema()
 *	Manually set the value for the semaphore count
 *
 * Use with care; if you strand someone on the queue your system
 * will start to act funny.  If DEBUG is on, it'll probably panic.
 */
void
set_sema(sema_t *s, int cnt)
{
	s->s_count = cnt;
}

/*
 * p_sema_v_lock()
 *	Atomically transfer from a spinlock to a semaphore
 */
int
p_sema_v_lock(sema_t *s, pri_t p, lock_t *l)
{
	struct thread *t;
	spl_t spl;

	ASSERT_DEBUG(cpu.pc_locks == 1, "p_sema_v: bad lock count");
	ASSERT_DEBUG(s->s_lock.l_lock == 0, "p_sema_v: deadlock");

	/*
	 * Take semaphore lock.  If count is high enough, release
	 * semaphore and lock now.
	 */
	spl = p_lock(&s->s_lock, SPLHI);
	s->s_count -= 1;
	if (s->s_count >= 0) {
		v_lock(&s->s_lock, spl);
		v_lock(l, SPL0);
		return(0);
	}

	/*
	 * We're going to sleep.  Add us at the tail of the
	 * queue, and relinquish the processor.
	 */
	t = curthread;
	t->t_wchan = s;
	t->t_intr = 0;
	t->t_nointr = (p == PRIHI);
	q_sema(s, t);
	p_lock(&runq_lock, SPLHI);
	v_lock(&s->s_lock, SPLHI);
	v_lock(l, SPLHI);
	t->t_state = TS_SLEEP;
	ATOMIC_DEC(&num_run);
	swtch();

	/*
	 * We're back.  If we have an event pending, give up on the
	 * semaphore and return the right result.
	 */
	t->t_nointr = 0;
	if (t->t_intr) {
		ASSERT_DEBUG(t->t_wchan == 0, "p_sema: intr w. wchan");
		ASSERT_DEBUG(p != PRIHI, "p_sema: intr w. PRIHI");
		if (p != PRICATCH) {
			longjmp(t->t_qsav, 1);
		}
		return(1);
	}

	/*
	 * We were woken up to be next with the semaphore.  Our waker
	 * did the reference count update, so we just return.
	 */
	return(0);
}

/*
 * cunsleep()
 *	Try to remove a process from a sema queue
 *
 * Returns 1 on busy mutex; 0 for success
 */
int
cunsleep(struct thread *t)
{
	spl_t s;
	sema_t *sp;

	ASSERT_DEBUG(t->t_wchan, "cunsleep: zero wchan");

	/*
	 * Try for mutex on semaphore
	 */
	sp = t->t_wchan;
	s = cp_lock(&sp->s_lock, SPLHI);
	if (s == -1) {
		return(1);
	}

	/*
	 * Get thread off queue
	 */
	dq_sema(sp, t);

	/*
	 * Null out wchan to be safe.  Sema count was updated
	 * by dq_sema().  Flag that he didn't get the semaphore.
	 */
	t->t_wchan = 0;
	t->t_intr = 1;

	/*
	 * Success
	 */
	v_lock(&sp->s_lock, s);
	return(0);
}
@


1.9
log
@Add counter for number of SRUN processes
@
text
@d11 1
a13 1
extern uint num_run;
@


1.8
log
@Use size-specific atomic ops
@
text
@d13 1
d167 1
d327 1
@


1.7
log
@Keep PRIHI sleepers from being interrupted
@
text
@d30 1
a30 1
	ATOMIC_INC(&cpu.pc_locks);
d55 1
a55 1
	ATOMIC_INC(&cpu.pc_locks);
d71 1
a71 1
	ATOMIC_DEC(&cpu.pc_locks);
@


1.6
log
@Fix -Wall warnings
@
text
@d160 1
d172 1
d319 1
d331 1
@


1.5
log
@Fix enqueueing to sema.  Revamp t_wchan and t_state, and add
more sanity checks.
@
text
@d136 1
d191 1
d252 1
d289 1
d350 1
@


1.4
log
@Rewrite cp_sema(); completely hosed
@
text
@d97 1
a97 1
		struct thread *t2 = s->s_sleepq->t_tl;
d99 4
a102 3
		t->t_hd = t2->t_hd;
		t->t_tl = t2;
		t2->t_tl->t_hd = t;
d127 3
d163 1
d221 4
a224 1
		dq_sema(s, t = s->s_sleepq);
d317 1
@


1.3
log
@Fix some windows where lock was fiddled without interrupts
blocked.
@
text
@a11 2
#define LOCK_HELD (0x8000)	/* Private flag p_sema<->cp_sema */

d142 1
a142 8
	if (p & LOCK_HELD) {
		p &= ~LOCK_HELD;
	} else {
		spl_t spl;

		spl = p_lock(&s->s_lock, SPLHI);
		ASSERT_DEBUG(spl == SPL0, "p_sema: lock held");
	}
d155 1
a155 1
	t->t_intr = 0;
a184 1
int
d188 1
d193 5
a197 4
	ASSERT_DEBUG(spl == SPL0, "cp_sema: lock held");
	if (s->s_count <= 0) {
		v_lock(&s->s_lock, SPL0);
		return(-1);
d199 2
a200 1
	return(p_sema(s, PRIHI|LOCK_HELD));
@


1.2
log
@Fix up handling for interrupted p_sema's
@
text
@a24 2
	ASSERT_DEBUG(l->l_lock == 0, "p_lock: deadlock");
	l->l_lock = 1;
d27 2
d30 3
a32 1
	cpu.pc_locks += 1;
d43 7
d51 3
d56 3
a58 1
	return(p_lock(l, s));
d73 1
a73 1
	cpu.pc_locks -= 1;
@


1.1
log
@Initial revision
@
text
@d124 1
d150 1
d160 1
a160 1
	if (EVENT(t) && (p < PRIHI)) {
d162 3
a164 2
		if (p == PRICATCH) {
			return(1);
d166 1
a166 1
		longjmp(t->t_qsav, 1);
d276 2
a277 1
	ASSERT_DEBUG(s->s_lock.l_lock == 0, "p_sema: deadlock");
d297 1
d308 1
a308 1
	if (EVENT(t) && (p < PRIHI)) {
d310 5
a314 3
		if (p == PRICATCH)
			return(1);
		longjmp(t->t_qsav, 1);
d353 1
a353 1
	 * by dq_sema().
d356 1
d361 1
@
