head	1.2;
access;
symbols;
locks; strict;
comment	@ * @;


1.2
date	94.12.21.05.29.59;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.07.06.04.43.43;	author vandys;	state Exp;
branches;
next	;


desc
@Process state interface definitions
@


1.2
log
@pstat() enhancements
@
text
@#ifndef _PSTAT_H
#define _PSTAT_H
/*
 * pstat.h
 *	Definitions for pstat() process status query syscall
 */
#include <sys/types.h>
#include <sys/sched.h>
#include <sys/perm.h>
#include <sys/syscall.h>
#include <mach/trap.h>
#include <mach/isr.h>

/*
 * process status struct
 */
struct pstat_proc {
	ulong psp_pid;		/* PID of process */
	char psp_cmd[8];	/* Command name */
	uint psp_nthread;	/* Number of threads under process */
	uint psp_nsleep;	/* Number of threads sleeping */
	uint psp_nrun;		/*  ...runnable */
	uint psp_nonproc;	/*  ...on a CPU */
	ulong psp_usrcpu;	/* Total CPU time used in user mode */
	ulong psp_syscpu;	/*  ...in kernel */
	struct prot psp_prot;	/* Process protection info */
	uchar psp_moves;	/* Does this process move in the list? */
};

/*
 * System type identification string
 */
#define PS_SYSID 16

/*
 * configuration/kernel status struct
 */
struct pstat_kernel {
	ulong psk_memory;	/* Size of system RAM in bytes */
	uint psk_ncpu;		/* Number of CPUs */
	ulong psk_freemem;	/* Bytes of free memory */
	struct time psk_uptime;	/* How long has the system been up? */
	uint psk_runnable;	/* Number of runnable threads */
};

/*
 * pstat status request types
 */
#define PSTAT_PROC 0
#define PSTAT_PROCLIST 1
#define PSTAT_KERNEL 2

/*
 * pstat()
 *	Get status information out of the kernel
 *
 * You pass it a status type, two arguments, a pointer to a status
 * structure and the sizeof() the struct.  This last allows the struct
 * to grow but still be binary compatible with old executables.
 */
extern int pstat(uint, uint, void *, uint);

#endif /* _PSTAT_H */
@


1.1
log
@Initial revision
@
text
@d8 5
d15 1
a15 1
 * pstat struct
d17 11
a27 9
struct pstat {
	ulong ps_pid;		/* PID of process */
	char ps_cmd[8];		/* Command name */
	uint ps_nthread;	/* # threads under process */
	uint ps_nsleep,		/* # threads sleeping */
		ps_nrun,	/*  ...runnable */
		ps_nonproc;	/*  ...on a CPU */
	ulong ps_usrcpu,	/* Total CPU time used in user mode */
		ps_syscpu;	/*  ...in kernel */
d31 23
d55 1
a55 1
 *	Get process status
d57 3
a59 3
 * You pass it a pstat struct, a count of such structs, and the sizeof()
 * the struct.  This last allows the struct to grow but still be binary
 * compatible with old executables.
d61 1
a61 1
extern int pstat(struct pstat *, uint, uint);
@
