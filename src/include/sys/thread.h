/*
 * thread.h
 *	Definitions of a stream of execution
 */
#ifndef _THREAD_H
#define _THREAD_H

#include <sys/param.h>
#include <sys/types.h>
#include <mach/setjmp.h>
#include <mach/machreg.h>
#include <sys/mutex.h>

struct thread {
	pid_t t_pid;		/* ID of thread */
	jmp_buf t_kregs;	/* Saved thread kernel state */
	struct trapframe
		*t_uregs;	/* User state on kernel stack */
	struct proc *t_proc;	/* Our process */
	char *t_kstack;		/* Base of our kernel stack */
	char *t_ustack;		/*  ...user stack for this thread */
	struct sched *t_runq;	/* Node in scheduling tree */
	uchar t_runticks;	/* # ticks left for this proc to run */
	uchar t_state;		/* State of process (see below) */
	uchar t_flags;		/* Misc. flags */
	uchar t_oink;		/* # times we ate our whole CPU quanta */
	struct thread		/* Run queue list */
		*t_hd, *t_tl,
		*t_next;	/* List of threads under a process */
	sema_t *t_wchan;	/* Semaphore we're asleep on */
	ushort t_intr;		/*  ...flag that we were interrupted */
	ushort t_nointr;	/*  ...flag non-interruptable */
	sema_t t_msgwait;	/* Semaphore slept on waiting for I/O */
	jmp_buf t_qsav;		/* Vector for interrupting p_sema */
	voidfun t_probe;	/* When probing user memory */
	char t_err[ERRLEN];	/* Error from last syscall */
	ulong t_syscpu,		/* Ticks in system and user mode */
		t_usrcpu;
	char t_evsys[EVLEN];	/* Event from system */
	char t_evproc[EVLEN];	/*  ...from user process */
	sema_t t_evq;		/* Queue when waiting to send */
	struct percpu		/* CPU running on for TS_ONPROC */
		*t_eng;
	struct fpu *t_fpu;	/* Saved FPU state, if any */
};

/*
 * Macros for fiddling fields
 */
#define EVENT(t) ((t)->t_evsys[0] || (t)->t_evproc[0])

/*
 * Bits in t_flags
 */
#define T_RT (0x1)		/* Thread is real-time priority */
#define T_BG (0x2)		/*  ... background priority */
#define T_KERN (0x4)		/* Thread is running in kernel mode */
#define T_FPU (0x8)		/* Thread has new state in the FPU */

/*
 * Values for t_state
 */
#define TS_SLEEP (1)		/* Thread sleeping */
#define TS_RUN (2)		/* Thread waiting for CPU */
#define TS_ONPROC (3)		/* Thread running */
#define TS_DEAD (4)		/* Thread dead/dying */

/*
 * Max value for t_oink.  This changes how much memory we will keep
 * of high CPU usage.
 */
#define T_MAX_OINK (32)

#ifdef KERNEL
extern void dup_stack(struct thread *, struct thread *, voidfun);
#endif

#endif /* _THREAD_H */
