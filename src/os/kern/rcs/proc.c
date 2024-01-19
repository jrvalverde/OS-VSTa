head	1.40;
access;
symbols
	V1_3_1:1.27
	V1_3:1.24
	V1_2:1.23
	V1_1:1.21
	V1_0:1.18;
locks; strict;
comment	@ * @;


1.40
date	95.01.04.06.05.46;	author vandys;	state Exp;
branches;
next	1.39;

1.39
date	94.12.28.21.52.35;	author vandys;	state Exp;
branches;
next	1.38;

1.38
date	94.12.23.05.13.01;	author vandys;	state Exp;
branches;
next	1.37;

1.37
date	94.12.19.05.49.14;	author vandys;	state Exp;
branches;
next	1.36;

1.36
date	94.11.30.23.24.56;	author vandys;	state Exp;
branches;
next	1.35;

1.35
date	94.11.16.19.36.07;	author vandys;	state Exp;
branches;
next	1.34;

1.34
date	94.11.05.10.06.03;	author vandys;	state Exp;
branches;
next	1.33;

1.33
date	94.10.12.04.01.33;	author vandys;	state Exp;
branches;
next	1.32;

1.32
date	94.10.05.17.57.46;	author vandys;	state Exp;
branches;
next	1.31;

1.31
date	94.08.30.19.48.40;	author vandys;	state Exp;
branches;
next	1.30;

1.30
date	94.07.06.04.44.56;	author vandys;	state Exp;
branches;
next	1.29;

1.29
date	94.06.08.00.12.23;	author vandys;	state Exp;
branches;
next	1.28;

1.28
date	94.05.21.21.44.45;	author vandys;	state Exp;
branches;
next	1.27;

1.27
date	94.04.26.21.37.42;	author vandys;	state Exp;
branches;
next	1.26;

1.26
date	94.04.19.03.16.05;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	94.04.19.00.27.10;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	94.03.23.21.55.22;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	93.12.14.21.37.17;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	93.12.09.06.16.36;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	93.10.02.01.24.31;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	93.08.18.05.26.07;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.06.30.19.55.12;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.04.09.17.13.50;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.04.01.18.45.57;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.03.31.04.37.29;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.03.27.00.33.04;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.03.24.00.37.45;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.03.22.23.21.13;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.03.18.18.21.15;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.03.05.23.52.44;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.03.03.23.18.14;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.02.26.15.15.57;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.02.19.21.41.38;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.02.10.18.10.48;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.09.17.09.07;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.03.20.16.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.02.15.16.01;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.01.15.43.28;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.52.42;	author vandys;	state Exp;
branches;
next	;


desc
@Process creation/deletion/initialization
@


1.40
log
@Fix leak on portref hash
@
text
@/*
 * proc.c
 *	Routines for creating/deleting processes
 */
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/vas.h>
#include <sys/pview.h>
#include <sys/pset.h>
#include <sys/percpu.h>
#include <sys/boot.h>
#include <sys/vm.h>
#include <sys/sched.h>
#include <sys/param.h>
#include <hash.h>
#include <sys/fs.h>
#include <sys/malloc.h>
#include <sys/assert.h>

extern void setrun(), dup_stack();
extern struct sched sched_root;
extern lock_t runq_lock;

ulong npid_free = (ulong)-1;	/* # PIDs free in pool */
ulong pid_nextfree = 0L;	/* Next free PID number */
struct proc *allprocs = 0;	/* List of all procs */
sema_t pid_sema;		/* Mutex for PID pool and proc lists */

uint nthread;			/* # threads currently in existence */

struct hash *pid_hash;		/* Mapping PID->proc */

/*
 * mkview()
 *	Create a boot task pview
 */
static struct pview *
mkview(uint pfn, void *vaddr, uint pages, struct vas *vas)
{
	struct pview *pv;
	struct pset *ps;
	extern struct pset *physmem_pset();

	ps = physmem_pset(pfn, pages);
	pv = MALLOC(sizeof(struct pview), MT_PVIEW);
	pv->p_set = ps;
	ref_pset(ps);
	pv->p_vaddr = vaddr;
	pv->p_len = pages;
	pv->p_off = 0;
	pv->p_vas = vas;
	pv->p_next = vas->v_views;
	vas->v_views = pv;
	pv->p_prot = 0;
	return(pv);
}

/*
 * add_proclist()
 *	Add process to list of all processes
 */
static void
add_proclist(struct proc *p)
{
	if (allprocs == 0) {
		p->p_allnext = p->p_allprev = p;
	} else {
		p->p_allnext = allprocs;
		p->p_allprev = allprocs->p_allprev;
		allprocs->p_allprev->p_allnext = p;
		allprocs->p_allprev = p;
	}
	allprocs = p;
}

/*
 * bootproc()
 *	Given a boot process image, throw together a proc for it
 */
static void
bootproc(struct boot_task *b)
{
	struct proc *p;
	struct thread *t;
	struct pview *pv;
	struct vas *vas;
	extern void boot_regs();
	extern void *alloc_zfod_vaddr();

	/*
	 * Get a proc, make him sys/sys with the usual values for
	 * the various fields.
	 */
	p = MALLOC(sizeof(struct proc), MT_PROC);
	bzero(p, sizeof(struct proc));
	zero_ids(p->p_ids, PROCPERMS);
	p->p_ids[0].perm_len = 2;
	p->p_ids[0].perm_id[0] = 1;
	p->p_ids[0].perm_id[1] = 1;

	/*
	 * Grant him root, but leave it disabled.  Those who really
	 * need it will know how to turn it on.
	 */
	p->p_ids[1].perm_len = 0;
	PERM_DISABLE(&p->p_ids[1]);

	/*
	 * Default protection: require sys/sys to touch us.
	 */
	p->p_prot.prot_len = 2;
	p->p_prot.prot_default = 0;
	p->p_prot.prot_id[0] = 1;
	p->p_prot.prot_id[1] = 1;
	p->p_prot.prot_bits[0] = 0;
	p->p_prot.prot_bits[1] = P_PRIO|P_SIG|P_KILL|P_STAT|P_DEBUG;

	/*
	 * Initialize other fields
	 */
	init_sema(&p->p_sema);
	p->p_runq = sched_node(&sched_root);
	p->p_pgrp = alloc_pgrp();
	p->p_children = alloc_exitgrp(p);
	p->p_parent = alloc_exitgrp(0); ref_exitgrp(p->p_parent);

	/*
	 * Set up the single thread for the proc
	 */
	t = MALLOC(sizeof(struct thread), MT_THREAD);
	bzero(t, sizeof(struct thread));
	t->t_kstack = MALLOC(KSTACK_SIZE, MT_KSTACK);
	t->t_ustack = (void *)USTACKADDR;
	p->p_threads = t;
	boot_regs(t, b);
	t->t_runq = sched_thread(p->p_runq, t);
	init_sema(&t->t_msgwait);
	t->t_proc = p;
	init_sema(&t->t_evq);
	t->t_state = TS_SLEEP;	/* -> RUN in setrun() */

	/*
	 * The vas for the proc
	 */
	vas = MALLOC(sizeof(struct vas), MT_VAS);
	vas->v_views = 0;
	vas->v_flags = VF_MEMLOCK | VF_BOOT;
	init_lock(&vas->v_lock);
	hat_initvas(vas);
	p->p_vas = vas;

	/*
	 * The views of text and data (and BSS)
	 */
	pv = mkview(b->b_pfn, b->b_textaddr, b->b_text, vas);
	pv->p_prot |= PROT_RO;
	(void)mkview(b->b_pfn + b->b_text, b->b_dataaddr, b->b_data, vas);

	/*
	 * Stack is ZFOD
	 */
	ASSERT(alloc_zfod_vaddr(vas, btorp(UMINSTACK), (void *)USTACKADDR),
		"bootproc: no stack");

	/*
	 * Get their PIDs
	 */
	p->p_pid = allocpid();
	t->t_pid = allocpid();
	join_pgrp(p->p_pgrp, p->p_pid);

	/*
	 * Add process to list of all processes and hash
	 */
	add_proclist(p);
	hash_insert(pid_hash, p->p_pid, p);

	/*
	 * Leave him hanging around ready to run
	 */
	setrun(t);
}

/*
 * refill_pids()
 *	When the PID pool runs out, replenish it
 *
 * Called with PID mutex held.
 */
static void
refill_pids(void)
{
	static pid_t rotor = 0L;
	pid_t pnext, pid;
	struct proc *p, *pstart;
	struct thread *t = 0;

retry:
	pnext = -1;
	p = pstart = allprocs;
	do {
		if (rotor <= 0) {
			rotor = 200L;	/* Where to scan from */
		}
		pid = p->p_pid;
		do {
			/*
			 * We're not interested in ones below; we're trying
			 * to build a range starting at rotor
			 */
			if (pid < rotor) {
				continue;
			}

			/*
			 * Collision with rotor
			 */
			if (pid == rotor) {
				/*
				 * The worst of all cases--we collided and
				 * there's * one above.  Our whole range is
				 * shot.  Advance the rotor and try again.
				 */
				if (++rotor == pnext) {
					++rotor;
					goto retry;
				}
				continue;
			}

			/*
			 * If we've found a new lower bound for our range,
			 * record it.
			 */
			if (pid < pnext) {
				pnext = pid;
			}

			/*
			 * Advance to next PID in threads
			 */
			if (!t) {
				t = p->p_threads;
			} else {
				t = t->t_next;
			}
			if (t) {
				pid = t->t_pid;
			}
		} while (t);
		p = p->p_allnext;
	} while (p != pstart);

	/*
	 * Update our pool range; set the rotor to one above the
	 * highest.
	 */
	npid_free = pnext - rotor;
	pid_nextfree = rotor;
	rotor = pnext+1;
}

/*
 * allocpid()
 *	Allocate a new PID
 *
 * We keep a range of free process IDs memorized.  When we exhaust it,
 * we scan for a new range.
 */
pid_t
allocpid(void)
{
	pid_t pid;

	/*
	 * If we're out, scan for new ones
	 */
	if (npid_free == 0) {
		refill_pids();
	}

	/*
	 * Take next free PID
	 */
	npid_free -= 1;
	pid = pid_nextfree++;

	return(pid);
}

/*
 * fork_thread()
 *	Launch a new thread within the same process
 */
pid_t
fork_thread(voidfun f)
{
	struct proc *p = curthread->t_proc;
	struct thread *t;
	void *ustack;
	pid_t npid;
	extern void *alloc_zfod();

	/*
	 * Do an unlocked increment of the thread count.  The limit
	 * is thus approximate; worth it for a faster thread launch?
	 */
	if (nthread >= NPROC) {
		return(err(ENOMEM));
	}

	/*
	 * Get a user stack first
	 */
	ustack = alloc_zfod(p->p_vas, btop(UMINSTACK));
	if (!ustack) {
		return(err(ENOMEM));
	}

	/*
	 * Allocate thread structure, set up its fields
	 */
	t = MALLOC(sizeof(struct thread), MT_THREAD);

	/*
	 * Most stuff we can just copy from the current thread
	 */
	*t = *curthread;

	/*
	 * Get him his own PID
	 */
	p_sema(&pid_sema, PRIHI);
	npid = t->t_pid = allocpid();
	v_sema(&pid_sema);

	/*
	 * He needs his own kernel stack; user stack was attached above
	 */
	t->t_kstack = MALLOC(KSTACK_SIZE, MT_KSTACK);
	t->t_ustack = ustack;
	dup_stack(curthread, t, f);

	/*
	 * Initialize
	 */
	init_sema(&t->t_msgwait);
	t->t_usrcpu = t->t_syscpu = 0L;
	t->t_evsys[0] = t->t_evproc[0] = '\0';
	init_sema(&t->t_evq);
	t->t_err[0] = '\0';
	t->t_runq = sched_thread(p->p_runq, t);
	t->t_uregs = 0;
	t->t_state = TS_SLEEP;
	t->t_fpu = 0;
	t->t_oink = 0;

	/*
	 * Add new guy to the proc's list
	 */
	p_sema(&p->p_sema, PRIHI);
	t->t_next = p->p_threads;
	p->p_threads = t;
	v_sema(&p->p_sema);

	/*
	 * He's for real now
	 */
	ATOMIC_INC(&nthread);

	/*
	 * Set him running
	 */
	setrun(t);

	/*
	 * Old thread returns with new thread's PID as its return value
	 */
	return (npid);
}

/*
 * init_proc()
 *	Set up stuff for process management
 */
void
init_proc(void)
{
	struct boot_task *b;
	int x;

	init_sema(&pid_sema);
	pid_hash = hash_alloc(NPROC/4);
	for (b = boot_tasks, x = 0; x < nboot_task; ++b, ++x) {
		bootproc(b);
	}
}

/*
 * fork()
 *	Fork out an entirely new process
 */
pid_t
fork(void)
{
	struct thread *tnew, *told = curthread;
	struct proc *pold = told->t_proc, *pnew;
	pid_t npid;
	extern struct vas *fork_vas();

	/*
	 * Check thread limit here
	/*
	 * Allocate new structures
	 */
	tnew = MALLOC(sizeof(struct thread), MT_THREAD);
	bzero(tnew, sizeof(struct thread));
	pnew = MALLOC(sizeof(struct proc), MT_PROC);
	bzero(pnew, sizeof(struct proc));

	/*
	 * Get new thread
	 */
	tnew->t_kstack = MALLOC(KSTACK_SIZE, MT_KSTACK);
	tnew->t_flags = told->t_flags;
	init_sema(&tnew->t_msgwait);
	init_sema(&tnew->t_evq);
	tnew->t_state = TS_SLEEP;	/* -> RUN in setrun() */
	tnew->t_ustack = (void *)USTACKADDR;

	/*
	 * Get new PIDs for process and initial thread.  Insert
	 * PID into hash.
	 */
	p_sema(&pid_sema, PRIHI);
	npid = pnew->p_pid = allocpid();
	tnew->t_pid = allocpid();
	v_sema(&pid_sema);

	/*
	 * Get new proc, copy over fields as appropriate
	 */
	tnew->t_proc = pnew;
	p_sema(&pold->p_sema, PRIHI);
	bcopy(pold->p_ids, pnew->p_ids, sizeof(pold->p_ids));
	init_sema(&pnew->p_sema);
	pnew->p_prot = pold->p_prot;
	pnew->p_threads = tnew;
	pnew->p_vas = fork_vas(tnew, pold->p_vas);
	pnew->p_runq = sched_node(pold->p_runq->s_up);
	tnew->t_runq = sched_thread(pnew->p_runq, tnew);
	fork_ports(&pold->p_sema, pold->p_open, pnew->p_open, PROCOPENS);
	pnew->p_nopen = pold->p_nopen;
	pnew->p_handler = pold->p_handler;
	pnew->p_pgrp = pold->p_pgrp; join_pgrp(pold->p_pgrp, npid);
	pnew->p_parent = pold->p_children; ref_exitgrp(pnew->p_parent);
	bcopy(pold->p_cmd, pnew->p_cmd, sizeof(pnew->p_cmd));
	v_sema(&pold->p_sema);
	pnew->p_children = alloc_exitgrp(pnew);

	/*
	 * Duplicate stack now that we have a viable thread/proc
	 * structure.
	 */
	dup_stack(told, tnew, 0);

	/*
	 * Now that we're ready, make PID known globally
	 */
	p_sema(&pid_sema, PRIHI);
	hash_insert(pid_hash, npid, pnew);
	add_proclist(pnew);
	v_sema(&pid_sema);

	/*
	 * Now he's real
	 */
	ATOMIC_INC(&nthread);

	/*
	 * Leave him runnable
	 */
	setrun(tnew);
	return(npid);
}

/*
 * free_proc()
 *	Routine to tear down and free proc
 */
static void
free_proc(struct proc *p)
{
	/*
	 * Close both server and client open ports
	 */
	close_ports(p->p_ports, PROCPORTS);
	close_portrefs(p->p_open, PROCOPENS);
	if (p->p_prefs) {
		ASSERT_DEBUG(hash_size(p->p_prefs) == 0,
			"free_proc: p_prefs not empty");
		hash_dealloc(p->p_prefs);
#ifdef DEBUG
		p->p_prefs = 0;
#endif
	}

	/*
	 * Delete us from the "allproc" list
	 */
	p_sema(&pid_sema, PRIHI);
	p->p_allnext->p_allprev = p->p_allprev;
	p->p_allprev->p_allnext = p->p_allnext;
	if (allprocs == p) {
		ASSERT_DEBUG(p->p_allnext != p, "free_proc: empty");
		allprocs = p->p_allnext;
	}

	/*
	 * Unhash our PID
	 */
	hash_delete(pid_hash, p->p_pid);
	v_sema(&pid_sema);

	/*
	 * Depart process group
	 */
	leave_pgrp(p->p_pgrp, p->p_pid);

	/*
	 * Clean our our vas
	 */
	if (p->p_vas->v_flags & VF_DMA) {
		pages_release(p);
	}
	free_vas(p->p_vas);

	/*
	 * Release proc storage
	 */
	FREE(p, MT_PROC);
}

/*
 * do_exit()
 *	The exit() system call, really
 *
 * Tricky once we tear down our kernel stack, because we can no longer
 * use stack variables once this happens.  The trivial way to deal with
 * this is to declare them "register" and hope the declaration is honored.
 * I'm trying to avoid this by using just the percpu "curthread" value.
 *
 * GCC tends to *know* what exit() means, so we name it do_exit() and
 * avoid the issue.
 */
do_exit(int code)
{
	struct thread *t = curthread, *t2, **tp;
	struct proc *p = t->t_proc;
	struct sched *prunq = p->p_runq;
	int last;

	/*
	 * Let debugger take a crack, if configured
	 */
	PTRACE_PENDING(p, PD_EXIT, 0);

	/*
	 * Remove our thread from the process hash list
	 */
	p_sema(&p->p_sema, PRIHI);
	tp = &p->p_threads;
	for (t2 = p->p_threads; t2; t2 = t2->t_next) {
		if (t2 == t) {
			*tp = t->t_next;
			break;
		}
		tp = &t2->t_next;
	}
	ASSERT(t2, "do_exit: lost thread");
	last = (p->p_threads == 0);

	/*
	 * Accumulate CPU in proc
	 */
	p->p_usr += t->t_usrcpu;
	p->p_sys += t->t_syscpu;

	v_sema(&p->p_sema);

	/*
	 * Let through anybody waiting to signal
	 */
	while (blocked_sema(&t->t_evq)) {
		v_sema(&t->t_evq);
	}

	/*
	 * Tear down the thread's user stack if not last.  If it's
	 * last, it'll get torn down with the along with the rest
	 * of the vas.
	 */
	if (!last) {
		remove_pview(p->p_vas, t->t_ustack);
	} else {
#ifdef DEBUG
		if (p->p_vas->v_flags & VF_BOOT) {
			printf("Boot process %d dies\n", p->p_pid);
			dbg_enter();
		}
#endif
		/*
		 * Detach from our exit groups
		 */
		noparent_exitgrp(p->p_children);
		post_exitgrp(p->p_parent, p, code);
		deref_exitgrp(p->p_parent);

		/*
		 * If last thread gone, tear down process.
		 */
		free_proc(p);
	}

	/*
	 * Don't let our CPU be taken away while we try to clean up
	 */
	NO_PREEMPT();

	/*
	 * Clear out thread's t_runq, and proc's if this is the
	 * last thread.
	 */
	free_sched_node(t->t_runq);
	if (last) {
		free_sched_node(prunq);
	}

	/*
	 * Free kernel stack once we've switched to our idle stack.
	 * Can't use local variables after this!
	 */
	idle_stack();
	FREE(curthread->t_kstack, MT_KSTACK);

	/*
	 * One less runable thread
	 */
	ATOMIC_DEC(&num_run);

	/*
	 * Drop FPU if in use
	 */
	if (curthread->t_fpu) {
		fpu_disable(0);
		FREE(curthread->t_fpu, MT_FPU);
	}

	/*
	 * Free thread, switch to new work
	 */
	FREE(curthread, MT_THREAD);
	curthread = 0;
	ATOMIC_DEC(&nthread);
	p_lock(&runq_lock, SPLHI);
	PREEMPT_OK();	/* Can't preempt now with runq_lock held */
	for (;;) {
		swtch();
		ASSERT(0, "do_exit: swtch returned");
	}
}

/*
 * pfind()
 *	Find a process, return it locked
 */
struct proc *
pfind(pid_t pid)
{
	struct proc *p;

	p_sema(&pid_sema, PRIHI);
	p = hash_lookup(pid_hash, pid);
	if (p) {
		/*
		 * Lock him
		 */
		p_sema(&p->p_sema, PRIHI);
	}
	v_sema(&pid_sema);
	return(p);
}

/*
 * waits()
 *	Get next exit() event from our exit group
 */
waits(struct exitst *w, int block)
{
	struct proc *p = curthread->t_proc;
	struct exitst *e;
	int x;

	/*
	 * Get next event.  We will vector out on interrupted system
	 * call, so a NULL return here simply means there are no
	 * children on which to wait.
	 */
	e = wait_exitgrp(p->p_children, block);
	if (e == 0) {
		return(err(ESRCH));
	}

	/*
	 * Copy out the status.  Hide a field which they don't
	 * need to see.  No big security risk, but helps provide
	 * determinism.
	 */
	if (w) {
		e->e_next = 0;
		if (copyout(w, e, sizeof(struct exitst))) {
			return(err(EFAULT));
		}
	}

	/*
	 * Record PID, free memory, return PID as value of syscall
	 */
	x = e->e_code;
	FREE(e, MT_EXITST);
	return(x);
}
@


1.39
log
@Fix location of num_run update on exit; needs to be after point
where we might preempt
@
text
@d499 8
@


1.38
log
@Use .h def
@
text
@a590 5
	 * One less runable thread
	 */
	ATOMIC_DEC(&num_run);

	/*
d637 9
a649 2
	FREE(curthread, MT_THREAD);
	curthread = 0;
d654 2
@


1.37
log
@Add release of proc sema as ports are duplicated
@
text
@a22 1
extern uint num_run;
@


1.36
log
@Fix counting of number of threads
@
text
@d23 1
a440 1

d453 1
a453 2
	bzero(&pnew->p_ports, sizeof(pnew->p_ports));
	fork_ports(pold->p_open, pnew->p_open, PROCOPENS);
d484 1
a484 3
	p_lock(&runq_lock, SPLHI);
	lsetrun(tnew);
	v_lock(&runq_lock, SPL0);
a501 8
	 * Clean our our vas
	 */
	if (p->p_vas->v_flags & VF_DMA) {
		pages_release(p);
	}
	free_vas(p->p_vas);

	/*
d524 8
d590 5
@


1.35
log
@Tidy up semaphore count handling, add assertions.  Convert
atomic ops so routine matches data element size.
@
text
@d305 2
a306 1
	 * Get a user stack first
d308 1
a308 2
	ustack = alloc_zfod(p->p_vas, btop(UMINSTACK));
	if (!ustack) {
d313 1
a313 2
	 * Do an unlocked increment of the thread count.  The limit
	 * is thus approximate; worth it for a faster thread launch?
d315 2
a316 2
	if (nthread >= NPROC) {
		remove_pview(p->p_vas, ustack);
a318 1
	ATOMIC_INC(&nthread);
d367 5
d412 2
d476 5
@


1.34
log
@Add counters to track CPU hogs
@
text
@d29 1
a29 1
int nthread;			/* # threads currently in existence */
d139 1
a139 1
	init_sema(&t->t_evq); set_sema(&t->t_evq, 1);
d352 1
a352 1
	init_sema(&t->t_evq); set_sema(&t->t_evq, 1);
d422 1
a422 1
	init_sema(&tnew->t_evq); set_sema(&tnew->t_evq, 1);
@


1.33
log
@Initialize CPU usage fields for a proc
@
text
@d358 1
d412 1
d414 1
a420 3
	tnew->t_hd = tnew->t_tl = tnew->t_next = 0;
	tnew->t_wchan = 0;
	tnew->t_nointr = tnew->t_intr = 0;
a421 4
	tnew->t_probe = 0;
	tnew->t_err[0] = '\0';
	tnew->t_usrcpu = tnew->t_syscpu = 0L;
	tnew->t_evsys[0] = tnew->t_evproc[0] = '\0';
a424 1
	tnew->t_fpu = 0;
a449 2
	pnew->p_prefs = 0;
	pnew->p_flags = 0;
a456 6
	pnew->p_event[0] = '\0';
	pnew->p_usr = pnew->p_sys = 0L;
#ifdef PROC_DEBUG
	bzero(&pnew->p_dbg, sizeof(struct pdbg));
	bzero(&pnew->p_dbgr, sizeof(struct dbg_regs));
#endif
@


1.32
log
@Add FPU support
@
text
@d465 1
@


1.31
log
@Keep PRIHI sleepers from being interrupted
@
text
@d357 1
d429 1
d643 4
@


1.30
log
@Fix mutexing for allprocs, tidy up return value handling
Add new proc flags field
@
text
@d419 1
a419 1
	tnew->t_intr = 0;
@


1.29
log
@Convert "all process" list to circular, doubly-linked
@
text
@d434 1
a434 1
	pnew->p_pid = allocpid();
d454 1
d457 1
a457 1
	pnew->p_pgrp = pold->p_pgrp; join_pgrp(pold->p_pgrp, pnew->p_pid);
d478 2
a479 1
	hash_insert(pid_hash, pnew->p_pid, pnew);
d483 1
a483 1
	 * Add to "all procs" list
a485 6
	add_proclist(pnew);
	npid = pnew->p_pid;

	/*
	 * Leave him runnable
	 */
@


1.28
log
@Only trap to kdb on death of *process*
@
text
@d59 18
d175 1
a175 2
	p->p_allnext = allprocs;
	allprocs = p;
d195 1
a195 1
	struct proc *p;
d200 2
a201 1
	for (p = allprocs; p; p = p->p_allnext) {
d251 2
a252 1
	}
d484 1
a484 2
	pnew->p_allnext = allprocs;
	allprocs = pnew;
a501 2
	struct proc *p2, **pp;

d520 5
a524 7
	pp = &allprocs;
	for (p2 = allprocs; p2; p2 = p2->p_allnext) {
		if (p2 == p) {
			*pp = p->p_allnext;
			break;
		}
		pp = &p2->p_allnext;
a525 1
	ASSERT(p2, "free_proc: lost proc");
@


1.27
log
@Trap to kernel debugger on death of boot server
@
text
@a554 6
#ifdef DEBUG
	if (p->p_vas->v_flags & VF_BOOT) {
		printf("Boot process %d dies\n", p->p_pid);
		dbg_enter();
	}
#endif
d593 6
@


1.26
log
@Fix sched node memory leak; add explicit preemption inhibit
@
text
@d129 1
a129 1
	vas->v_flags = VF_MEMLOCK;
d555 6
@


1.25
log
@Convert to MALLOC
@
text
@d547 1
d607 14
a623 1
	ATOMIC_INC(&cpu.pc_locks);	/* To avoid preemption */
d634 1
a634 1
	ATOMIC_DEC(&cpu.pc_locks);	/* swtch() will handle dispatch */
@


1.24
log
@Add boot procs to hash so they can be found
@
text
@a13 1
#include <alloc.h>
d17 1
d45 1
a45 1
	pv = malloc(sizeof(struct pview));
d76 1
a76 1
	p = malloc(sizeof(struct proc));
d112 1
a112 1
	t = malloc(sizeof(struct thread));
d114 1
a114 1
	t->t_kstack = malloc(KSTACK_SIZE);
d127 1
a127 1
	vas = malloc(sizeof(struct vas));
d306 1
a306 1
	t = malloc(sizeof(struct thread));
d323 1
a323 1
	t->t_kstack = malloc(KSTACK_SIZE);
d390 2
a391 2
	tnew = malloc(sizeof(struct thread));
	pnew = malloc(sizeof(struct proc));
d396 1
a396 1
	tnew->t_kstack = malloc(KSTACK_SIZE);
d528 1
a528 1
	free(p);
d611 2
a612 2
	free(curthread->t_kstack);
	free(curthread);
d684 1
a684 1
	free(e);
@


1.23
log
@Add tap for exiting process
@
text
@d155 1
a155 1
	 * Add process to list of all processes
d159 1
@


1.22
log
@Add ptrace hooks
@
text
@d549 5
@


1.21
log
@Source reorg
@
text
@d442 4
@


1.20
log
@Add missing handling for p_handler field
@
text
@d14 1
a14 1
#include <lib/alloc.h>
d16 1
a16 1
#include <lib/hash.h>
@


1.19
log
@Change exit() -> do_exit() to make gcc happy
@
text
@d435 1
@


1.18
log
@GCC warning cleanup
@
text
@d526 2
a527 2
 * exit()
 *	The exit() system call
d533 3
d537 1
a537 1
exit(int code)
d555 1
a555 1
	ASSERT(t2, "exit: lost thread");
d612 1
a612 1
		ASSERT(0, "exit: swtch returned");
@


1.17
log
@Add non-blocking waits(), convert to pid_t
@
text
@d607 4
a610 1
	swtch();
@


1.16
log
@Make boot procs immune to page stealing
@
text
@d175 2
a176 2
	static ulong rotor = 200L;	/* Where to scan from */
	ulong pnext, pid;
d178 1
d181 1
a181 1
	pnext = (ulong)-1;	/* XXX assumes two's complement */
d183 3
a185 2
		struct thread *t = 0;

d250 1
a250 1
ulong
d253 1
a253 1
	ulong pid;
d275 1
d281 1
a281 1
	uint npid;
d378 1
d383 1
a383 1
	uint npid;
d440 1
d615 1
a615 1
pfind(ulong pid)
d635 1
a635 1
waits(struct exitst *w)
d646 1
a646 1
	e = wait_exitgrp(p->p_children);
@


1.15
log
@Fiddle with treatment of t_state to help debugging
@
text
@d129 1
@


1.14
log
@Add p_cmd to hold a short program name per process
@
text
@d122 1
d332 1
d392 1
a392 2
	tnew->t_hd = tnew->t_tl = tnew;
	tnew->t_next = 0;
d401 1
a401 1
	tnew->t_state = TS_RUN;
@


1.13
log
@Add a second, disabled, ability to boot processes--the root ID.
Can be activated by login when needed to forge UIDs for a newly-
logging-in process.
@
text
@d431 1
@


1.12
log
@Left a & in front of a pointer, so copied back piece of stack
@
text
@d82 11
a92 1
	init_sema(&p->p_sema);
d99 5
@


1.11
log
@Exit code is a more interesting return value for waits()
@
text
@d635 1
a635 1
		if (copyout(w, &e, sizeof(struct exitst))) {
@


1.10
log
@Need to use the proc pointer before we free it!
@
text
@d616 1
a616 1
	ulong pid;
d643 1
a643 1
	pid = e->e_pid;
d645 1
a645 1
	return(pid);
@


1.9
log
@Add exit group handling
@
text
@d556 7
d565 1
a565 1
		free_proc(t->t_proc);
a566 7

	/*
	 * Detach from our exit groups
	 */
	noparent_exitgrp(p->p_children);
	post_exitgrp(p->p_parent, p, code);
	deref_exitgrp(p->p_parent);
@


1.8
log
@Incomplete initialization of t_evq
@
text
@d91 2
d361 1
d414 2
a415 1
	join_pgrp(pold->p_pgrp, pnew->p_pid);
d417 1
d438 1
d445 1
a554 1
		printf("exit pid %d\n", p->p_pid); dbg_enter();
d562 7
d606 40
@


1.7
log
@Re-order exit sequence; free() could lower spl to 0, which
would then allow us to fall into swtch() with runq_lock held
but interrupts enabled.  We could then interrupt against an
interrupt setrun()'ing somebody.
Er. *deadlock* against...
@
text
@d104 1
d310 1
a310 1
	init_sema(&t->t_evq);
d381 1
a381 1
	init_sema(&tnew->t_evq);
@


1.6
log
@Get rid of dbg_enter on exit()
@
text
@d547 1
d558 1
d561 2
d569 1
a569 2
	free(curthread);
	curthread = 0;
@


1.5
log
@Add t_intr field
@
text
@a508 2
	printf("Exit.\n"); dbg_enter();

@


1.4
log
@Use ref_pset() interface
@
text
@d374 1
@


1.3
log
@Get fork() working
@
text
@d47 1
a47 1
	ps->p_refs += 1;
@


1.2
log
@Get tfork() working
@
text
@d20 1
a20 1
extern void setrun();
d90 1
d132 1
a259 1
	extern void dup_stack();
d361 6
a368 1
	tnew = malloc(sizeof(struct thread));
d381 11
a395 1
	pnew = malloc(sizeof(struct proc));
d409 1
d419 1
a419 2
	 * Get new PIDs for process and initial thread.  Insert
	 * PID into hash.
a421 2
	pnew->p_pid = allocpid();
	tnew->t_pid = allocpid();
d426 7
d435 2
a436 1
	setrun(tnew);
d457 3
d481 5
@


1.1
log
@Initial revision
@
text
@d251 1
a251 1
fork_thread(void)
a266 1
	t->t_ustack = ustack;
d299 2
a300 1
	dup_stack(curthread, t);
d310 2
d399 1
a399 1
	dup_stack(told, tnew);
@
