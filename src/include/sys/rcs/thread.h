head	1.7;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.7
date	94.11.05.08.38.35;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.10.05.17.57.14;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.08.30.19.48.40;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.09.17.11.14;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.09.17.08.40;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.01.15.45.21;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.17.40;	author vandys;	state Exp;
branches;
next	;


desc
@Thread state
@


1.7
log
@Add variable for tracking heavy CPU usage
@
text
@/*
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
@


1.6
log
@Add FPU support
@
text
@d25 2
a26 1
	ushort t_flags;		/* Misc. flags */
d67 6
@


1.5
log
@Keep PRIHI sleepers from being interrupted
@
text
@d43 1
d54 4
a57 3
#define T_RT (1)		/* Thread is real-time priority */
#define T_BG (2)		/*  ... background priority */
#define T_KERN (4)		/* Thread is running in kernel mode */
@


1.4
log
@Convert to pid_t
@
text
@d30 2
a31 1
	int t_intr;		/*  ...flag that we were interrupted */
@


1.3
log
@Add flag so thread can tell when interrupted from a p_sema
@
text
@d15 1
a15 1
	ulong t_pid;		/* ID of thread */
@


1.2
log
@Add prototype for dup_stack()
@
text
@d30 1
@


1.1
log
@Initial revision
@
text
@d63 4
@
