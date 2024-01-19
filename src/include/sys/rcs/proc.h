head	1.14;
access;
symbols
	V1_3_1:1.11
	V1_3:1.11
	V1_2:1.11
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.14
date	94.12.21.05.31.48;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.07.06.04.45.29;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.06.08.00.12.23;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.12.14.20.37.01;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.12.13.02.20.41;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.12.10.19.14.36;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.12.09.06.19.23;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.51.47;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.09.17.11.25;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.27.00.30.25;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.08.23.02.51;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.03.23.14.58;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.15.13.14;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.16.46;	author vandys;	state Exp;
branches;
next	;


desc
@Proc and thread state data structures
@


1.14
log
@Remove unneeded proc struct flags
@
text
@#ifndef _PROC_H
#define _PROC_H
/*
 * proc.h
 *	Per-process data
 */
#define PROC_DEBUG	/* Define for process debugging support */
#define PSTAT		/*  ...for process status query */

#include <sys/perm.h>
#include <sys/param.h>
#include <llist.h>
#include <sys/mutex.h>
#include <sys/wait.h>
#include <mach/machreg.h>

/*
 * Permission bits for current process
 */
#define P_PRIO 1	/* Can set process priority */
#define P_SIG 2		/*  ... deliver events */
#define P_KILL 4	/*  ... terminate */
#define P_STAT 8	/*  ... query status */
#define P_DEBUG 16	/*  ... debug */

/*
 * Description of a process group
 */
struct pgrp {
	uint pg_nmember;	/* # PID's in group */
	uint pg_nelem;		/* # PID's in pg_members[] array */
	ulong *pg_members;	/* Pointer to linear array */
	sema_t pg_sema;		/* Mutex for pgrp */
};
#define PG_GROWTH (20)		/* # slots pg_members[] grows by */

#ifdef PROC_DEBUG
/*
 * Communication area between debugger and slave
 */
struct pdbg {
	port_t pd_port;		/* Debugger serves this port */
	port_name pd_name;	/*  ...portname for port */
	uint pd_flags;		/* See below */
};

/* Bits in pd_flags */
#define PD_ALWAYS 1		/* Stop at next chance */
#define PD_SYSCALL 2		/*  ...before & after syscalls */
#define PD_EVENT 4		/*  ...when receiving an event */
#define PD_EXIT 8		/*  ...when exiting */
#define PD_BPOINT 16		/*  ...breakpoint reached */
#define PD_EXEC 32		/*  ...exec done (new addr space) */
#define PD_CONNECTING 0x8000	/* Slave has connect in progress */

/*
 * Values for m_op of a debug message.  The sense of the operation
 * is further defined by m_arg and m_arg1.
 */
#define PD_SLAVE 300		/* Slave ready for commands */
#define PD_RUN 301		/* Run */
#define PD_STEP 302		/* Run one instruction, then break */
#define PD_BREAK 303		/* Set/clear breakpoint */
#define PD_RDREG 304		/* Read registers */
#define PD_WRREG 305		/*  ...write */
#define PD_MASK 306		/* Set mask */
#define PD_RDMEM 307		/* Read memory */
#define PD_WRMEM 308		/*  ...write */
#define PD_MEVENT 309		/* Read/write event */

/*
 * Handy macro for checking if we should drop into debug slave
 * mode.
 */
extern void ptrace_slave(char *, uint);

#define PTRACE_PENDING(p, fl, ev) \
	if ((p)->p_dbg.pd_flags & ((fl) | PD_ALWAYS)) { \
		ptrace_slave(ev, (fl) & (p)->p_dbg.pd_flags); \
	}

#else

/*
 * Just stub this for non-debugging kernel
 */
#define PTRACE_PENDING(p, fl, ev)

#endif /* PROC_DEBUG */

/*
 * The actual per-process state
 */
struct proc {
	pid_t p_pid;		/* Our process ID */
	struct perm		/* Permissions granted process */
		p_ids[PROCPERMS];
	sema_t p_sema;		/* Semaphore on proc structure */
	struct prot p_prot;	/* Protections of this process */
	struct thread		/* List of threads in this process */
		*p_threads;
	struct vas *p_vas;	/* Virtual address space of process */
	struct sched *p_runq;	/* Scheduling node for all threads */
	struct port		/* Ports this proc owns */
		*p_ports[PROCPORTS];
	struct portref		/* "files" open by this process */
		*p_open[PROCOPENS];
	struct hash		/* Portrefs attached to our ports */
		*p_prefs;
	ulong p_nopen;		/*  ...# currently open */
	struct proc		/* Linked list of all processes */
		*p_allnext, *p_allprev;
	ulong p_sys, p_usr;	/* Cumulative time for all prev threads */
	voidfun p_handler;	/* Handler for events */
	struct pgrp		/* Process group for proc */
		*p_pgrp;
	struct exitgrp		/* Exit groups */
		*p_children,	/*  ...the one our children use */
		*p_parent;	/*  ...the one we're a child of */
	char p_cmd[8];		/* Command name (untrusted) */
	char p_event[ERRLEN];	/* Event which killed us */
#ifdef PROC_DEBUG
	struct pdbg p_dbg;	/* Who's debugging us (if anybody) */
	struct dbg_regs		/* Debug register state */
		p_dbgr;		/*  valid if T_DEBUG active */
#endif
};

#ifdef KERNEL
extern pid_t allocpid(void);
extern int alloc_open(struct proc *);
extern void free_open(struct proc *, int);
extern void join_pgrp(struct pgrp *, pid_t),
	leave_pgrp(struct pgrp *, pid_t);
extern struct pgrp *alloc_pgrp(void);

/*
 * notify() syscall and special value for thread ID to signal whole
 * process group instead.
 */
extern notify(pid_t, pid_t, char *, int);
#define NOTIFY_PG ((pid_t)-1)

#endif /* KERNEL */

#endif /* _PROC_H */
@


1.13
log
@Add pstat #ifdef, add new proc flags field
@
text
@d110 1
a110 2
	ushort p_nopen;		/*  ...# currently open */
	ushort p_flags;		/* Miscellaneous flags */
a127 6

/*
 * Bits in p_flags
 */
#define PF_MOVED 0x1		/* Proc moved in allnext/allprev */
				/* (i.e., is a pstat() user) */
@


1.12
log
@Convert "all process" list to circular, doubly-linked
@
text
@d7 2
a8 1
#define PROC_DEBUG /* Define for process debugging support */
d111 1
d129 6
@


1.11
log
@Add support for debugging a new process
@
text
@d111 1
a111 1
		*p_allnext;
@


1.10
log
@Add prototype, add arg so debugger can see why ptrace_slave
called
@
text
@d52 1
@


1.9
log
@Add read/write reg as two ops
@
text
@d73 2
d77 1
a77 1
		ptrace_slave(ev); \
@


1.8
log
@Add ptrace support
@
text
@d62 6
a67 4
#define PD_REGS 304		/* Get/put registers */
#define PD_MASK 305		/* Set mask */
#define PD_RDMEM 306		/* Read memory */
#define PD_WRMEM 307		/*  ...write */
@


1.7
log
@Source reorg
@
text
@d7 2
d14 1
d36 49
d116 5
@


1.6
log
@Convert to pid_t
Add p_event to show last killing event
@
text
@d9 1
a9 1
#include <lib/llist.h>
@


1.5
log
@Add a small field to hold a program name
@
text
@d37 1
a37 1
	ulong p_pid;		/* Our process ID */
d63 1
d67 1
a67 1
extern ulong allocpid(void);
d70 2
a71 2
extern void join_pgrp(struct pgrp *, ulong),
	leave_pgrp(struct pgrp *, ulong);
d78 2
a79 2
extern notify(ulong, ulong, char *, int);
#define NOTIFY_PG ((ulong)-1)
@


1.4
log
@Move waits() stuff out to <sys/wait.h>
@
text
@d62 1
@


1.3
log
@New fields to support exitgrp/waits() functionality.  Function
prototypes for exitgrp handling.
@
text
@d11 1
a33 23
 * Status a child leaves behind on exit()
 */
struct exitst {
	ulong e_pid;		/* PID of exit() */
	int e_code;		/* Argument to exit() */
	ulong e_usr, e_sys;	/* CPU time in user and sys */
	struct exitst *e_next;	/* Next in list */
};

/*
 * An exit group.  All children of the same parent belong to the
 * same exit group.  When children exit, they will leave an exit
 * status message linked here if the parent is still alive.
 */
struct exitgrp {
	struct proc *e_parent;	/* Pointer to parent of group */
	sema_t e_sema;		/* Sema bumped on each exit */
	struct exitst *e_stat;	/* Status of each exit() */
	ulong e_refs;		/* # references (parent + children) */
	lock_t e_lock;		/* Mutex for fiddling */
};

/*
a77 9

/*
 * Functions for fiddling exit groups
 */
extern struct exitgrp *alloc_exitgrp(struct proc *);
extern void deref_exitgrp(struct exitgrp *);
extern void noparent_exitgrp(struct exitgrp *);
extern void post_exitgrp(struct exitgrp *, struct proc *, int);
extern struct exitst *wait_exitgrp(struct exitgrp *);
@


1.2
log
@Add process group support
@
text
@d33 23
d81 3
d101 10
a110 1
#endif
@


1.1
log
@Initial revision
@
text
@d22 11
d56 2
d64 11
@
