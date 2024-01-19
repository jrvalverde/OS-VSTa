head	1.14;
access;
symbols
	V1_3_1:1.11
	V1_3:1.10
	V1_2:1.8
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.14
date	94.12.19.05.51.08;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.08.30.19.48.40;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.08.25.00.57.26;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.04.19.00.27.10;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.04.06.18.41.24;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.02.22.00.29;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.12.09.06.16.36;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.09.17.14.04;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.17.18.17.28;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.26.15.16.15;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.09.17.10.27;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.15.16.16;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.50.36;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of process events (the replacement of signals)
@


1.14
log
@Clean up some warnings
@
text
@/*
 * event.c
 *	Code for delivering events to a thread or process
 *
 * Both threads and processes have IDs.  An event sent to a process
 * results in the event being delivered to each thread under the
 * process.  An event can also be sent to a specific thread.
 *
 * A process has two distinct incoming streams
 * of events--a process-generated one and a system-generated one.
 * The system events take precedence.  For process events, a sender
 * will sleep until the target process has accepted a current event and
 * (if he has a handler registered) returned from the handler.  System
 * events simply overwrite each other; when the process finally gets
 * the event, it only sees the latest event received.
 *
 * t_wchan is used to get a thread interrupted from a sleep.  Because
 * the runq_lock is already held when the semaphore indicated by t_wchan
 * needs to be manipulated, a potential deadlock exists.  The delivery
 * code knows how to back out from this situation and retry.
 *
 * Mutexing is accomplished via the destination process' p_sema and
 * the global runq_lock.  p_sema (the field, not the function) is needed
 * to keep the process around while its threads are manipulated.  runq_lock
 * is used to mutex a consistent thread state while delivering the
 * effects of an event.
 */
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/mutex.h>
#include <sys/fs.h>
#include <sys/malloc.h>
#include <hash.h>
#include <sys/assert.h>

extern lock_t runq_lock;
extern struct hash *pid_hash;
extern struct proc *pfind();

/*
 * signal_thread()
 *	Send an event to a thread
 *
 * The process mutex is held by the caller.
 */
static int
signal_thread(struct thread *t, char *event, int is_sys)
{
	extern void nudge(), lsetrun();

	if (!is_sys) {
		if (p_sema(&t->t_evq, PRICATCH)) {
			return(err(EINTR));
		}
	}

	/*
	 * Take lock, place event in appropriate place
	 */
retry:	p_lock(&runq_lock, SPLHI);
	strcpy((is_sys ? t->t_evsys : t->t_evproc), event);

	/*
	 * As appropriate, kick him awake
	 */
	switch (t->t_state) {
	case TS_SLEEP:		/* Interrupt sleep */
		if (t->t_nointr == 0) {
			if (cunsleep(t)) {
				v_lock(&runq_lock, SPL0);
				goto retry;
			}
			lsetrun(t);
		}
		break;

	case TS_ONPROC:		/* Nudge him */
		nudge(t->t_eng);
		break;

	case TS_RUN:		/* Nothing to do for these */
	case TS_DEAD:
		break;

	default:
		ASSERT(0, "signal_thread: unknown state");
	}
	v_lock(&runq_lock, SPL0);
	return(0);
}

/*
 * notifypg()
 *	Send an event to a process group
 */
static int
notifypg(struct proc *p, char *event)
{
	ulong *l, *lp;
	uint x, nelem;
	struct pgrp *pg = p->p_pgrp;

	p_sema(&pg->pg_sema, PRIHI);

	/*
	 * Get the list of processes to hit while holding the
	 * group's semaphore.
	 */
	nelem = pg->pg_nmember;
	l = MALLOC(pg->pg_nmember * sizeof(ulong), MT_PGRP);
	lp = pg->pg_members;
	for (x = 0; x < nelem; ++x) {
		while (*lp == 0)
			++lp;
		l[x] = *lp++;
	}

	/*
	 * Release the semaphores, then go about trying to
	 * signal the processes.
	 */
	v_sema(&pg->pg_sema);
	v_sema(&p->p_sema);
	for (x = 0; x < nelem; ++x) {
		notify2(l[x], 0L, event);
	}
	FREE(l, MT_PGRP);
	return(0);
}

/*
 * notify2()
 *	Most of the work for doing an event notification
 *
 * If arg_proc is 0, it means the current process.  If arg_thread is
 * 0, it means all threads under the named proc.
 */
int
notify2(pid_t arg_proc, pid_t arg_thread, char *evname)
{
	struct proc *p;
	int x, error = 0;
	struct thread *t;

	if (arg_proc == 0) {
		/*
		 * Our process isn't too hard
		 */
		p = curthread->t_proc;
		p_sema(&p->p_sema, PRILO);
	} else {
		/*
		 * Look up given PID
		 */
		p = pfind(arg_proc);
		if (!p) {
			return(err(ESRCH));
		}
	}

	/*
	 * See if we're allowed to signal him
	 */
	x = perm_calc(curthread->t_proc->p_ids, PROCPERMS, &p->p_prot);
	if (!(x & P_SIG)) {
		error = err(EPERM);
		goto out;
	}

	/*
	 * Magic value for thread means signal process group instead
	 */
	if (arg_thread == NOTIFY_PG) {
		return(notifypg(p, evname));
	}

	/*
	 * If this is a single thread signal, hunt the thread down
	 */
	if (arg_thread != 0) {
		for (t = p->p_threads; t; t = t->t_next) {
			if (t->t_pid == arg_thread) {
				if (signal_thread(t, evname, 0)) {
					error = -1;
					goto out;
				}
			}
		}
		/*
		 * Never found him
		 */
		error = err(ESRCH);
		goto out;
	}

	/*
	 * Otherwise hit each guy in a row
	 */
	for (t = p->p_threads; t; t = t->t_next) {
		if (signal_thread(t, evname, 0)) {
			error = -1;
			goto out;
		}
	}
out:
	v_sema(&p->p_sema);
	return(error);
}

/*
 * notify()
 *	Send a process event somewhere
 *
 * Wrapper to get event string into kernel
 */
int
notify(pid_t arg_proc, pid_t arg_thread, char *arg_msg, int arg_msglen)
{
	char evname[EVLEN];

	/*
	 * Get the string event name
	 */
	if (get_ustr(evname, sizeof(evname), arg_msg, arg_msglen)) {
		return(-1);
	}
	return(notify2(arg_proc, arg_thread, evname));
}

/*
 * selfsig()
 *	Send a signal to the current thread
 *
 * Used solely to deliver system exceptions.  Because it only delivers
 * to the current thread, it can assume that the thread is running (and
 * thus exists!)
 */
void
selfsig(char *ev)
{
	spl_t s;

	ASSERT(curthread, "selfsig: no thread");
	s = p_lock(&runq_lock, SPLHI);
	strcpy(curthread->t_evsys, ev);
	v_lock(&runq_lock, s);
}

/*
 * check_events()
 *	If there are events, deliver them
 */
void
check_events(void)
{
	struct thread *t = curthread;
	spl_t s;
	char event[EVLEN];
	extern void sendev();

	/*
	 * No thread or no events--nothing to do
	 */
	if (!t || !EVENT(t)) {
		return;
	}

	/*
	 * Take next events, act on them
	 */
	s = p_lock(&runq_lock, SPLHI);
	if (t->t_evsys[0]) {
		strcpy(event, t->t_evsys);
		t->t_evsys[0] = '\0';
		v_lock(&runq_lock, s);
		PTRACE_PENDING(t->t_proc, PD_EVENT, event);
		if (event[0])
			sendev(t, event);
		return;
	}
	if (t->t_evproc[0]) {
		strcpy(event, t->t_evproc);
		t->t_evproc[0] = '\0';
		v_lock(&runq_lock, s);
		PTRACE_PENDING(t->t_proc, PD_EVENT, event);
		if (event[0])
			sendev(t, event);
		v_sema(&t->t_evq);
		return;
	}
	v_lock(&runq_lock, s);
}

/*
 * join_pgrp()
 *	Join a process group
 */
void
join_pgrp(struct pgrp *pg, pid_t pid)
{
	void *e;
	ulong *lp;

	p_sema(&pg->pg_sema, PRIHI);

	/*
	 * If there's no more room now for members, grow the list
	 */
	if (pg->pg_nmember >= pg->pg_nelem) {
		e = MALLOC((pg->pg_nelem + PG_GROWTH) * sizeof(ulong),
			MT_PGRP);
		bcopy(pg->pg_members, e, pg->pg_nelem * sizeof(ulong));
		FREE(pg->pg_members, MT_PGRP);
		pg->pg_members = e;
		bzero(pg->pg_members + pg->pg_nelem,
			PG_GROWTH*sizeof(ulong));
		pg->pg_nelem += PG_GROWTH;
	}

	/*
	 * Insert in open slot
	 */
	for (lp = pg->pg_members; *lp; ++lp)
		;
	ASSERT_DEBUG(lp < (pg->pg_members + pg->pg_nelem),
		"join_pgrp: no slot");
	*lp = pid;
	pg->pg_nmember += 1;

	v_sema(&pg->pg_sema);
}

/*
 * leave_pgrp()
 *	Leave a process group
 */
void
leave_pgrp(struct pgrp *pg, pid_t pid)
{
	ulong *lp;

	p_sema(&pg->pg_sema, PRIHI);

	/*
	 * If we're the last, throw it out
	 */
	if (pg->pg_nmember == 1) {
		FREE(pg->pg_members, MT_PGRP);
		FREE(pg, MT_PGRP);
		return;
	}

	/*
	 * Otherwise leave the group, zero out our slot
	 */
	pg->pg_nmember -= 1;
	for (lp = pg->pg_members; *lp != pid; ++lp)
		;
	ASSERT_DEBUG(lp < (pg->pg_members + pg->pg_nelem),
		"leave_pgrp: no slot");
	*lp = 0;

	v_sema(&pg->pg_sema);
}

/*
 * alloc_pgrp()
 *	Allocate a new process group
 */
struct pgrp *
alloc_pgrp(void)
{
	struct pgrp *pg;
	uint len;

	pg = MALLOC(sizeof(struct pgrp), MT_PGRP);
	pg->pg_nmember = 0;
	len = PG_GROWTH * sizeof(ulong);
	pg->pg_members = MALLOC(len, MT_PGRP);
	bzero(pg->pg_members, len);
	pg->pg_nelem = PG_GROWTH;
	init_sema(&pg->pg_sema);
	return(pg);
}

/*
 * notify_handler()
 *	Register handler for notify() events
 */
int
notify_handler(voidfun handler)
{
	curthread->t_proc->p_handler = handler;
	return(0);
}
@


1.13
log
@Keep PRIHI sleepers from being interrupted
@
text
@a36 1
extern sema_t pid_sema;
d47 1
a47 1
static
d90 1
d97 1
a97 1
static
d139 1
d217 1
@


1.12
log
@Add signal handling syscall
@
text
@d70 6
a75 3
		if (cunsleep(t)) {
			v_lock(&runq_lock, SPL0);
			goto retry;
a76 1
		lsetrun(t);
@


1.11
log
@Convert to MALLOC
@
text
@d382 11
@


1.10
log
@Get rid of stale debug printf
@
text
@d33 1
a34 1
#include <alloc.h>
d109 1
a109 1
	l = malloc(pg->pg_nmember * sizeof(ulong));
d126 1
a126 1
	free(l);
d307 2
a308 1
		e = malloc((pg->pg_nelem + PG_GROWTH) * sizeof(ulong));
d310 1
a310 1
		free(pg->pg_members);
d345 2
a346 2
		free(pg->pg_members);
		free(pg);
d373 1
a373 1
	pg = malloc(sizeof(struct pgrp));
d376 1
a376 1
	pg->pg_members = malloc(len);
@


1.9
log
@Fix bogus assembly loop
@
text
@a115 1
	printf("sig pg %d members at 0x%x\n", nelem, l); dbg_enter();
@


1.8
log
@Add ptrace hooks
@
text
@a114 1
		x -= 1;
d116 1
@


1.7
log
@Source reorg
@
text
@d274 3
a276 1
		sendev(t, event);
d283 3
a285 1
		sendev(t, event);
@


1.6
log
@Convert to pid_t
@
text
@d33 2
a34 2
#include <lib/hash.h>
#include <lib/alloc.h>
@


1.5
log
@Need to zero initial memory for pgroup
@
text
@d138 1
a138 1
notify2(ulong arg_proc, ulong arg_thread, char *evname)
d215 1
a215 1
notify(ulong arg_proc, ulong arg_thread, char *arg_msg, int arg_msglen)
d293 1
a293 1
join_pgrp(struct pgrp *pg, ulong pid)
d331 1
a331 1
leave_pgrp(struct pgrp *pg, ulong pid)
@


1.4
log
@Some mistakes in handling of event string
@
text
@d367 1
d371 3
a373 1
	pg->pg_members = malloc(PG_GROWTH * sizeof(ulong));
@


1.3
log
@Get rid of dbg_enter in notify()
@
text
@d63 1
a63 1
	strcpy(is_sys ? t->t_evsys : t->t_evproc, event);
d274 1
a274 1
		sendev(t, t->t_evsys);
d278 1
a278 1
		strcpy(event, t->t_evsys);
d281 1
a281 1
		sendev(t, t->t_evproc);
@


1.2
log
@Add signaling of process groups
@
text
@a218 1
	printf("Notify\n"); dbg_enter();
@


1.1
log
@Initial revision
@
text
@d34 1
d92 42
a133 2
 * notify()
 *	Send a process event somewhere
d138 1
a138 1
notify(ulong arg_proc, ulong arg_thread, char *arg_msg, int arg_msglen)
a140 1
	char evname[EVLEN];
a143 8
	printf("Notify\n"); dbg_enter();
	/*
	 * Get the string event name
	 */
	if (get_ustr(evname, sizeof(evname), arg_msg, arg_msglen)) {
		return(-1);
	}

d170 7
d210 20
d287 88
@
