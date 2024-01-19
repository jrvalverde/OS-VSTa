head	1.26;
access;
symbols
	V1_3_1:1.22
	V1_3:1.22
	V1_2:1.21
	V1_1:1.20
	V1_0:1.17;
locks; strict;
comment	@ * @;


1.26
date	94.12.21.05.33.46;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	94.12.21.05.26.35;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	94.08.25.00.57.26;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	94.07.06.04.44.26;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	94.03.15.22.04.28;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	93.12.09.06.17.22;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	93.10.01.19.08.15;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	93.08.31.00.07.46;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.08.18.05.26.30;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.04.23.22.42.51;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.04.20.21.25.56;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.04.12.20.57.44;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.03.30.01.11.52;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.03.27.00.33.25;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.03.26.23.33.05;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.03.24.19.13.30;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.03.20.00.24.22;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.03.03.23.18.34;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.02.26.18.46.50;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.02.23.18.22.20;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.02.09.17.11.12;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.08.19.45.41;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.08.15.10.24;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.03.20.16.41;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.01.15.44.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.07.47;	author vandys;	state Exp;
branches;
next	;


desc
@Syscall handling
@


1.26
log
@Added 2nd arg to sched_op() to pass desired priority
@
text
@/*
 * syscall.c
 *	Tables and functions for doing system calls
 *
 * Much of this is somewhat portable, but is left here because
 * architectures exist in which sizes are not a uniform 32-bits,
 * and more glue is needed to pick up the arguments from the user
 * and pack them into a proper procedure call.
 */
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/percpu.h>
#include <sys/thread.h>
#include <sys/fs.h>
#include <mach/machreg.h>
#include <mach/gdt.h>
#include <sys/assert.h>

static int do_dbg_enter();

extern int msg_port(), msg_connect(), msg_accept(), msg_send(),
	msg_receive(), msg_reply(), msg_disconnect(), msg_err();
extern int do_exit(), fork(), fork_thread(), enable_io(), enable_isr(),
	mmap(), munmap(), strerror(), notify(), clone();
extern int page_wire(), page_release(), enable_dma(), time_get(),
	time_sleep(), exec(), waits(), perm_ctl(), set_swapdev(),
	run_qio(), set_cmd(), pageout(), getid(), unhash(),
	time_set(), ptrace(), nop(), msg_portname(), pstat();
extern int notify_handler(), sched_op();

struct syscall {
	intfun s_fun;
	int s_narg;
} syscalls[] = {
	{msg_port, 2},				/*  0 */
	{msg_connect, 2},			/*  1 */
	{msg_accept, 1},			/*  2 */
	{msg_send, 2},				/*  3 */
	{msg_receive, 2},			/*  4 */
	{msg_reply, 2},				/*  5 */
	{msg_disconnect, 1},			/*  6 */
	{msg_err, 3},				/*  7 */
	{do_exit, 1},				/*  8 */
	{fork, 0},				/*  9 */
	{fork_thread, 1},			/* 10 */
	{enable_io, 2},				/* 11 */
	{enable_isr, 2},			/* 12 */
	{mmap, 6},				/* 13 */
	{munmap, 2},				/* 14 */
	{strerror, 1},				/* 15 */
	{notify, 4},				/* 16 */
	{clone, 1},				/* 17 */
	{page_wire, 2},				/* 18 */
	{page_release, 1},			/* 19 */
	{enable_dma, 0},			/* 20 */
	{time_get, 1},				/* 21 */
	{time_sleep, 1},			/* 22 */
	{do_dbg_enter, 0},			/* 23 */
	{exec, 3},				/* 24 */
	{waits, 2},				/* 25 */
	{perm_ctl, 3},				/* 26 */
	{set_swapdev, 1},			/* 27 */
	{run_qio, 0},				/* 28 */
	{set_cmd, 1},				/* 29 */
	{pageout, 0},				/* 30 */
	{getid, 1},				/* 31 */
	{unhash, 2},				/* 32 */
	{time_set, 1},				/* 33 */
#ifdef PROC_DEBUG
	{ptrace, 2},				/* 34 */
#else
	{nop, 1},
#endif
	{msg_portname, 1},			/* 35 */
#ifdef PSTAT
	{pstat, 4},				/* 36 */
#else
	{nop, 1},
#endif
	{notify_handler, 1},			/* 37 */
	{sched_op, 2},				/* 38 */
};
#define NSYSCALL (sizeof(syscalls) / sizeof(struct syscall))
#define MAXARGS (6)

/*
 * do_dbg_enter()
 *	Enter debugger on DEBUG kernel, otherwise EINVAL
 */
static
do_dbg_enter(void)
{
#if defined(DEBUG) && defined(KDB)
	dbg_enter();
	return(0);
#else
	return(err(EINVAL));
#endif
}

/*
 * mach_flagerr()
 *	Flag that the current operation resulted in an error
 */
void
mach_flagerr(struct trapframe *f)
{
	f->eflags |= F_CF;
}

/*
 * syscall()
 *	Code to handle a trap for system services
 */
void
syscall(struct trapframe *f)
{
	int callnum, args[MAXARGS];
	struct syscall *s;

	/*
	 * Sanity check system call number
	 */
	callnum = f->eax;
	if ((callnum < 0) || (callnum >= NSYSCALL)) {
#ifdef DEBUG
		printf("Bad syscall # %d\n", callnum);
		dbg_enter();
#endif
		f->eax = err(EINVAL);
		return;
	}
	s = &syscalls[callnum];
	ASSERT_DEBUG(s->s_narg <= MAXARGS, "syscall: too many args");

	/*
	 * See if can get needed number of arguments
	 */
	if (copyin(f->esp + sizeof(ulong), args,
			s->s_narg * sizeof(int))) {
#ifdef DEBUG
		printf("Short syscall args\n");
		dbg_enter();
#endif
		f->eax = err(EFAULT);
		return;
	}

	/*
	 * Default to carry flag clear--no error
	 */
	f->eflags &= ~F_CF;

	/*
	 * Interrupted system calls vector here
	 */
	if (setjmp(curthread->t_qsav)) {
		err(EINTR);
		return;
	}

	/*
	 * Call function with needed number of arguments
	 */
	switch (s->s_narg) {
	case 0:
		f->eax = (*(s->s_fun))();
		break;
	case 1:
		f->eax = (*(s->s_fun))(args[0]);
		break;
	case 2:
		f->eax = (*(s->s_fun))(args[0], args[1]);
		break;
	case 3:
		f->eax = (*(s->s_fun))(args[0], args[1], args[2]);
		break;
	case 4:
		f->eax = (*(s->s_fun))(args[0], args[1], args[2],
			args[3]);
		break;
	case 5:
		f->eax = (*(s->s_fun))(args[0], args[1], args[2],
			args[3], args[4]);
		break;
	case 6:
		f->eax = (*(s->s_fun))(args[0], args[1], args[2],
			args[3], args[4], args[5]);
		break;
	default:
		ASSERT(0, "syscall: bad s_narg");
	}
}
@


1.25
log
@Add scheduler ops
@
text
@d81 1
a81 1
	{sched_op, 1},				/* 38 */
@


1.24
log
@Add signal handling syscall
@
text
@d29 1
a29 1
extern int notify_handler();
d76 1
a76 1
	{pstat, 3},				/* 36 */
d81 1
@


1.23
log
@Add pstat()
@
text
@d29 1
d80 1
@


1.22
log
@Add msg_portname()
@
text
@a20 2
/* #define SYSCALLTRACE /* */

d28 1
a28 1
	time_set(), ptrace(), nop(), msg_portname();
d74 5
a150 10
#ifdef SYSCALLTRACE
	{ int x;
	  printf("%d: syscall %d (", curthread->t_pid, callnum);
	  for (x = 0; x < s->s_narg; ++x) {
	    printf(" 0x%x", args[x]);
	  }
	  printf(" )\n");
	}
#endif

a189 7
#ifdef SYSCALLTRACE
	printf("%d:  --call %d returns 0x%x",
		curthread->t_pid, callnum, f->eax);
	printf(" / %s\n", (curthread->t_err[0]) ?
		curthread->t_err : "<none>");
	/* dbg_enter(); /* */
#endif
@


1.21
log
@Add ptrace() support
@
text
@d30 1
a30 1
	time_set(), ptrace(), nop();
d75 1
@


1.20
log
@Add time_set()
@
text
@d11 1
d30 1
a30 1
	time_set();
d70 5
@


1.19
log
@Fix # args to waits()
@
text
@d18 2
d28 2
a29 2
	run_qio(), set_cmd(), pageout(), getid(), unhash();
static int do_dbg_enter();
d68 1
@


1.18
log
@Change exit() -> do_exit() to make gcc happy
@
text
@d58 1
a58 1
	{waits, 1},				/* 25 */
@


1.17
log
@Implement KDB
@
text
@d22 1
a22 1
extern int exit(), fork(), fork_thread(), enable_io(), enable_isr(),
d41 1
a41 1
	{exit, 1},				/*  8 */
@


1.16
log
@unhash() syscall
@
text
@d77 1
a77 1
#ifdef DEBUG
@


1.15
log
@getid() syscall
@
text
@d26 1
a26 1
	run_qio(), set_cmd(), pageout(), getid();
d65 1
@


1.14
log
@Add pageout daemon interface
@
text
@d26 1
a26 1
	run_qio(), set_cmd(), pageout();
d64 1
@


1.13
log
@Add set_cmd() interface to the p_cmd field
@
text
@d26 1
a26 1
	run_qio(), set_cmd();
d63 1
@


1.12
log
@Add swap and qio interfaces
@
text
@d26 1
a26 1
	run_qio();
d62 1
@


1.11
log
@Add stuff so carry flag now reflects error in syscall
@
text
@d25 2
a26 1
	time_sleep(), exec(), waits(), perm_ctl();
d60 2
@


1.10
log
@Add perm_ctl() syscall
@
text
@d79 10
d125 5
@


1.9
log
@Add waits() syscall
@
text
@d25 1
a25 1
	time_sleep(), exec(), waits();
d58 1
@


1.8
log
@Go back to old way of passing syscall arguments; the caller
didn't have ebx available for saving return address anyway.
@
text
@d25 1
a25 1
	time_sleep(), exec();
d57 1
@


1.7
log
@Add kernel debugger call, as well as exec()
@
text
@d104 2
a105 1
	if (copyin(f->esp, args, s->s_narg * sizeof(int))) {
@


1.6
log
@Set up t_qsav vector for interrupted system calls
@
text
@d25 2
a26 1
	time_sleep();
d55 2
d60 15
@


1.5
log
@New arg for msg_port
@
text
@d106 8
@


1.4
log
@Add time syscalls
@
text
@d31 1
a31 1
	{msg_port, 1},				/*  0 */
@


1.3
log
@Add physical I/O support.  Also improve syscall trace output
@
text
@d24 2
a25 1
extern int page_wire(), page_release(), enable_dma();
d52 2
@


1.2
log
@Add argument--start address--for tfork()
@
text
@d24 1
d48 3
d94 1
a94 1
	  printf("Syscall %d pid %d:", callnum, curthread->t_pid);
d98 1
a98 1
	  printf("\n");
d134 2
a135 2
	printf("  --syscall %d pid %d returns 0x%x",
		callnum, curthread->t_pid, f->eax);
@


1.1
log
@Initial revision
@
text
@d39 1
a39 1
	{fork_thread, 0},			/* 10 */
d79 1
a79 2
	if (copyin(f->esp + sizeof(int), args,
			s->s_narg * sizeof(int))) {
@
