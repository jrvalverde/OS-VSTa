head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.5
date	94.02.01.23.23.21;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.06.30.19.52.49;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.20.53.59;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.09.17.10.50;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.08.19.06.17;	author vandys;	state Exp;
branches;
next	;


desc
@Process exit synch handling
@


1.5
log
@Add waits() proto
@
text
@#ifndef _WAIT_H
#define _WAIT_H
/*
 * wait.h
 *	Process exit coordination
 */
#include <sys/types.h>
#include <sys/param.h>

/*
 * Status a child leaves behind on exit()
 */
struct exitst {
	pid_t e_pid;		/* PID of exit() */
	int e_code;		/* Argument to exit() */
	ulong e_usr, e_sys;	/* CPU time in user and sys */
	struct exitst *e_next;	/* Next in list */
	char e_event[ERRLEN];	/* Name of event if killed */
};

/*
 * Encoded bits in e_code word
 */
#define _W_EV (0x10000)		/* Process died on event */

/*
 * System call function protoype
 */
extern int waits(struct exitst *, int);

#ifdef KERNEL
#include <sys/mutex.h>

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
 * Functions for fiddling exit groups
 */
extern struct exitgrp *alloc_exitgrp(struct proc *);
extern void deref_exitgrp(struct exitgrp *);
extern void noparent_exitgrp(struct exitgrp *);
extern void post_exitgrp(struct exitgrp *, struct proc *, int);
extern struct exitst *wait_exitgrp(struct exitgrp *, int);
extern pid_t parent_exitgrp(struct exitgrp *);

#else

/*
 * Prototypes for user/POSIX
 */
extern pid_t wait(int *),
	waitpid(pid_t, int *, int);

/*
 * Options to waitpid()
 */
#define WNOHANG (1)		/* Don't wait */
#define WUNTRACED (2)		/* Job control; ignored */

/*
 * Fiddling with the returned int
 */
#define WIFSIGNALED(x) ((x) & _W_EV)	/* Killed by event */
#define WTERMSIG(x) (((x) >> 8) & 0xFF)	/* Signal # */
#define WIFEXITED(x) (!WIFSIGNALED(x))	/* Called exit() */
#define WEXITSTATUS(x) ((x) & 0xFF)	/* Value passed to exit() */
#define WIFSTOPPED(x) (0)		/* No job control */
#define WSTOPSIG(x) (0)

#endif /* KERNEL */

#endif /* _WAIT_H */
@


1.4
log
@GCC warning cleanup
@
text
@d26 5
@


1.3
log
@Add prototype for new interface routine
@
text
@d18 1
a18 1
	int e_event[ERRLEN];	/* Name of event if killed */
@


1.2
log
@Add POSIX interface for waits()
@
text
@d50 1
@


1.1
log
@Initial revision
@
text
@d8 1
d14 1
a14 1
	ulong e_pid;		/* PID of exit() */
d18 1
d21 5
d49 25
a73 1
extern struct exitst *wait_exitgrp(struct exitgrp *);
@
