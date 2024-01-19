head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.3
	V1_2:1.3
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	94.10.05.17.57.14;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.19.03.19.40;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.12.12.22.45.39;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.15.07.35;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.16.03;	author vandys;	state Exp;
branches;
next	;


desc
@Per-CPU state information
@


1.5
log
@Add FPU support
@
text
@#ifndef _PERCPU_H
#define _PERCPU_H
/*
 * percpu.h
 *	Data structure which exists per CPU on the system
 */
#include <sys/types.h>

struct percpu {
	struct thread *pc_thread;	/* Thread CPU's running */
	uchar pc_num;			/* Sequential CPU ID */
	uchar pc_pri;			/* Priority running on CPU */
	ushort pc_locks;		/* # locks held by CPU */
	struct percpu *pc_next;		/* Next in list--circular */
	ulong pc_flags;			/* See below */
	ulong pc_ticks;			/* Ticks queued for clock */
	ulong pc_time[2];		/* HZ and seconds counting */
	int pc_preempt;			/* Flag that preemption needed */
	ulong pc_nopreempt;		/* > 0, preempt held off */
};

/*
 * Bits in pc_flags
 */
#define CPU_UP 0x1	/* CPU is online and taking work */
#define CPU_BOOT 0x2	/* CPU was the boot CPU for the system */
#define CPU_CLOCK 0x4	/* CPU is in clock handling code */
#define CPU_DEBUG 0x8	/* CPU hardware debugging active */
#define CPU_FP 0x10	/* CPU floating point unit present */

#ifdef KERNEL
extern struct percpu cpu;		/* Maps to percpu struct on each CPU */
#define curthread cpu.pc_thread
#define do_preempt cpu.pc_preempt
extern uint ncpu;			/* # CPUs on system */
extern struct percpu *nextcpu;		/* Rotor for preemption scans */

#define NO_PREEMPT() (cpu.pc_nopreempt += 1)
#define PREEMPT_OK() (cpu.pc_nopreempt -= 1)
#endif

#endif /* _PERCPU_H */
@


1.4
log
@Add explicit preemption prevention, add cleanup routine
for sched nodes
@
text
@d25 5
a29 4
#define CPU_UP 1	/* CPU is online and taking work */
#define CPU_BOOT 2	/* CPU was the boot CPU for the system */
#define CPU_CLOCK 4	/* CPU is in clock handling code */
#define CPU_DEBUG 8	/* CPU hardware debugging active */
@


1.3
log
@Add support flag for hardware debug resources
@
text
@d19 1
d36 3
@


1.2
log
@Add preemption flag to per-CPU structure; doesn't work as a global
when more than one engine.
@
text
@d27 1
@


1.1
log
@Initial revision
@
text
@d18 1
d31 1
@
