head	1.4;
access;
symbols;
locks; strict;
comment	@ * @;


1.4
date	94.12.23.05.13.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.12.21.05.25.15;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.11.05.10.06.03;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.07.06.04.43.53;	author vandys;	state Exp;
branches;
next	;


desc
@Process state query interface
@


1.4
log
@Use .h def
@
text
@/*
 * pstat.c
 *	pstat() system call for looking at kernel status info
 */
#include <sys/proc.h>
#ifdef PSTAT
#include <sys/thread.h>
#include <sys/pstat.h>
#include <sys/percpu.h>
#include <sys/assert.h>
#include <sys/fs.h>
#include <sys/sched.h>

extern sema_t pid_sema;
extern struct proc *allprocs;
extern uint size_base, size_ext;
extern uint freemem;

extern struct proc *pfind();
extern void uptime();

/*
 * get_pstat_proclist()
 *	Get a list of the requesting user's processes.
 *
 * Returns the number of processes in the list or an error code (-1)
 */
int
get_pstat_proclist(void *pst_info, uint pst_size)
{
	struct perm *perms = curthread->t_proc->p_ids;
	struct proc *p = allprocs;
	pid_t *pid_list = (pid_t *)pst_info;
	int max_ids;
	int x, i = 0;

	/*
	 * How many IDs can we return?
	 */
	max_ids = pst_size / sizeof(pid_t);

	/*
	 * Do this work with the pid_sema held
	 */
	p_sema(&pid_sema, PRILO);

	do {
		p_sema(&p->p_sema, PRIHI);

		/*
		 * Check whether we're allowed to see this process
		 */
		x = perm_calc(perms, PROCPERMS, &p->p_prot);
		if (x & P_STAT) {
			copyout(&pid_list[i++], &p->p_pid, sizeof(pid_t));
		}

		/*
		 * Step on to the next process in the list
 		 */
		v_sema(&p->p_sema);
		p = p->p_allnext;
	} while ((p != allprocs) && (i < max_ids));

	v_sema(&pid_sema);
	return(i);
}

/*
 * get_pstat_proc()
 *	Get details of a specific process
 */
int
get_pstat_proc(uint pid, void *pst_info, uint pst_size)
{
	struct pstat_proc psp;
	struct proc *p;
	struct thread *t;
	int x;
	
	if (pst_size > sizeof(struct pstat_proc)) {
		pst_size = sizeof(struct pstat_proc);
	}

	p = pfind((pid_t)pid);
	if (!p) {
		return(err(ESRCH));
	}

	/*
	 * Check whether we're allowed to see into this process
	 */
	x = perm_calc(curthread->t_proc->p_ids, PROCPERMS, &p->p_prot);
	if (!(x & P_STAT)) {
		v_sema(&p->p_sema);
		return(err(EPERM));
	}

	/*
	 * Record process-level goodies
	 */
	psp.psp_pid = p->p_pid;
	bcopy(p->p_cmd, psp.psp_cmd, sizeof(psp.psp_cmd));
	psp.psp_usrcpu = p->p_usr;
	psp.psp_syscpu = p->p_sys;
	psp.psp_prot = p->p_prot;

	/*
	 * Scan threads
	 */
	psp.psp_nthread = psp.psp_nsleep = psp.psp_nrun =
		psp.psp_nonproc = 0;
	for (t = p->p_threads; t; t = t->t_next) {
		/*
		 * Tally the number of threads, accumulate each
		 * thread's CPU usage
		 */
		psp.psp_nthread += 1;
		psp.psp_usrcpu += t->t_usrcpu;
		psp.psp_syscpu += t->t_syscpu;
		/*
		 * Tally # of threads in each state
		 */
		switch (t->t_state) {
		case TS_SLEEP:
			psp.psp_nsleep += 1;
			break;
		case TS_RUN:
			psp.psp_nrun += 1;
			break;
		case TS_ONPROC:
			psp.psp_nonproc += 1;
			break;
		}
	}
	v_sema(&p->p_sema);

	/*
	 * Copy out the process status info
	 */
	if (copyout((struct pstat_proc *)pst_info, &psp, pst_size) < 0) {
		return(-1);
	}

	return(0);
}

/*
 * get_pstat_kernel()
 *	Get system/kernel characteristic details
 *
 * We are passed a size parameter for the return information to allow us
 * to maintain binary compatibility in the future
 */
static int
get_pstat_kernel(void *pst_info, uint pst_size)
{
	struct pstat_kernel psk;
	
	if (pst_size > sizeof(struct pstat_kernel)) {
		pst_size = sizeof(struct pstat_kernel);
	}

	/*
	 * Fill our output structure with useful info
	 */
	psk.psk_memory = size_base + size_ext;
	psk.psk_ncpu = ncpu;
	psk.psk_freemem = freemem * NBPG;
	uptime(&psk.psk_uptime);
	psk.psk_runnable = num_run;

	/*
	 * Give the info back to the user
	 */
	if (copyout((struct pstat_kernel *)pst_info, &psk, pst_size) < 0) {
		return(-1);
	}

	return(0);
}

/*
 * pstat()
 *	System call handler
 */
int
pstat(uint ps_type, uint ps_arg, void *ps_info, uint ps_size)
{
	/*
	 * Decide what information we've been asked for
	 */
	switch(ps_type) {
	case PSTAT_PROC:
		return(get_pstat_proc(ps_arg, ps_info, ps_size));
	case PSTAT_PROCLIST:
		return(get_pstat_proclist(ps_info, ps_size));
	case PSTAT_KERNEL:
		return(get_pstat_kernel(ps_info, ps_size));
	default:
		/*
		 * We don't understand what we've been asked for
		 */
		return(err(EINVAL));
	}
}

#endif /* PSTAT */
@


1.3
log
@Add pstat enhancements
@
text
@d12 1
a17 1
extern uint num_run;
@


1.2
log
@Add counters to track CPU hogs
@
text
@d3 1
a3 11
 *	pstat() system call for looking at process status
 *
 * The big problem with pstat() is how allow a process to walk
 * along the list of processes when you don't know how long the
 * list even is.  The solution used here is to move the current
 * process to a new list position right after the last element
 * returned.  Thus, our own process' position in the allprocs
 * list indicates who should be dumped next.
 *
 * The current process is returned when we reach the end of the
 * list and wrap back to the beginning.
d11 1
d13 5
d19 2
a20 1
extern sema_t pid_sema;
d23 4
a26 2
 * pstat()
 *	System call handler
d29 1
a29 1
pstat(struct pstat *psp, uint npst, uint pst_size)
d31 5
a35 5
	struct proc *p, *thisproc = curthread->t_proc;
	struct thread *t;
	struct pstat ps;
	pid_t startpid = 0;
	uint count = 0;
d37 4
a40 5
	ASSERT_DEBUG(sizeof(ps.ps_cmd) <= sizeof(p->p_cmd),
		"pstat: p_cmd too small");
	if (pst_size > sizeof(struct pstat)) {
		pst_size = sizeof(struct pstat);
	}
d43 1
a43 4
	 * Fetch next process.  Hold PID semaphore, gather data
	 * on a single process, move us past him, and release.
	 * Iterate until we have the requested amount, or have
	 * wrapped back to the process we started with.
d45 2
d48 2
d51 1
a51 1
		 * Get next
d53 4
a56 2
		p_sema(&pid_sema, PRILO);
		p = thisproc->p_allnext;
d59 30
a88 6
		 * Record process-level goodies
		 */
		ps.ps_pid = p->p_pid;
		bcopy(p->p_cmd, ps.ps_cmd, sizeof(ps.ps_cmd));
		ps.ps_usrcpu = p->p_usr;
		ps.ps_syscpu = p->p_sys;
d90 5
a94 30
		/*
		 * Lock process, and scan threads
		 */
		ps.ps_nthread = ps.ps_nsleep = ps.ps_nrun =
			ps.ps_nonproc = 0;
		p_sema(&p->p_sema, PRIHI);
		for (t = p->p_threads; t; t = t->t_next) {
			/*
			 * Tally # of threads, accumulate each
			 * thread's CPU usage
			 */
			ps.ps_nthread += 1;
			ps.ps_usrcpu += t->t_usrcpu;
			ps.ps_syscpu += t->t_syscpu;

			/*
			 * Tally # of threads in each state
			 */
			switch (t->t_state) {
			case TS_SLEEP:
				ps.ps_nsleep += 1;
				break;
			case TS_RUN:
				ps.ps_nrun += 1;
				break;
			case TS_ONPROC:
				ps.ps_nonproc += 1;
				break;
			}
		}
d96 2
d99 8
a106 14
		/*
		 * Move our own process' position to reflect
		 * who we just scanned.  PF_MOVED is a hint to
		 * other users of pstat() that this process
		 * moves around.
		 */
		thisproc->p_allprev->p_allnext = thisproc->p_allnext;
		thisproc->p_allnext->p_allprev = thisproc->p_allprev;
		thisproc->p_allnext = p->p_allnext;
		thisproc->p_allprev = p;
		thisproc->p_flags |= PF_MOVED;
		p->p_allnext->p_allprev = thisproc;
		p->p_allnext = thisproc;
		v_sema(&pid_sema);
d108 26
a133 5
		/*
		 * Record pid
		 */
		if (startpid == 0) {
			startpid = ps.ps_pid;
d135 47
d183 18
d202 1
a202 1
		 * Copy out, advance to next
d204 2
a205 9
		if (copyout(psp, &ps, pst_size) < 0) {
			return(-1);
		}
		if (++count >= npst) {
			break;
		}
		psp = (struct pstat *)((char *)psp + pst_size);
	} while (ps.ps_pid != startpid);
	return(count);
@


1.1
log
@Initial revision
@
text
@d16 1
a21 1
#ifdef PSTAT
d36 1
a36 1
	uint count = 0, size;
@
