head	1.4;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.4
date	94.10.05.17.57.14;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.08.29.01.42.08;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.12.09.06.19.54;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.06.06;	author vandys;	state Exp;
branches;
next	;


desc
@Definition of i386 register set
@


1.4
log
@Add FPU support
@
text
@#ifndef _MACHREG_H
#define _MACHREG_H
/*
 * machreg.h
 *	Register definitions for i386
 */
#include <sys/types.h>
#include <sys/param.h>

/*
 * This is what is added to a user's stack when an event is
 * delivered.
 */
struct evframe {
	ulong ev_prevsp;		/* Points to ev_previp, actually */
	char ev_event[EVLEN];		/* Event delivered */
	ulong ev_previp;		/* Previous IP */
};

/*
 * This is the shape of the kernel stack after taking an interrupt
 * or exception.  For machine traps which don't provide an error
 * code, we push a 0 ourselves.  "traptype" is from sys/trap.h.
 * edi..eax are in pushal format.
 */
struct trapframe {
	ulong esds;
	ulong edi, esi, ebp, espdummy, ebx, edx, ecx, eax;
	ulong traptype;
	ulong errcode;
	ulong eip, ecs;
	ulong eflags;
	ulong esp, ess;
};
#define NREG (sizeof(struct trapframe) / sizeof(ulong))

/*
 * Tell if given descriptor is from user mode
 */
#define USERMODE(tf) (((tf)->ecs & 0x3) == 3)

/*
 * Bits in eflags
 */
#define	F_CF	0x00000001	/* carry */
#define	F_PF	0x00000004	/* parity */
#define	F_AF	0x00000010	/* BCD stuff */
#define	F_ZF	0x00000040	/* zero */
#define	F_SF	0x00000080	/* sign */
#define	F_TF	0x00000100	/* single step */
#define	F_IF	0x00000200	/* interrupts */
#define	F_DF	0x00000400	/* direction */
#define	F_OF	0x00000800	/* overflow */
#define	F_IOPL	0x00003000	/* IO privilege level */
#define	F_NT	0x00004000	/* nested task */
#define	F_RF	0x00010000	/* resume flag */
#define	F_VM	0x00020000	/* virtual 8086 */

/*
 * Bits in errcode when handling page fault
 */
#define EC_KERNEL 4	/* Fault from kernel mode */
#define EC_WRITE 2	/* Access was a write */
#define EC_PROT 1	/* Page valid, but access modes wrong */

#ifdef PROC_DEBUG
/*
 * This stuff is only needed in the context of proc.h, and only
 * then when process debugging is configured.
 *
 * WARNING: locore.s knows about this struct, so don't fiddle it
 * until you've looked at how locore.s uses it.
 */
struct dbg_regs {
	ulong dr[4];	/* DR0..3--linear addr to break on */
	ulong dr7;	/* Debug control register */
};
#endif /* PROC_DEBUG */

/*
 * Buffer for saving FPU state - matches the layout used by frestor
 */
struct fpu {
	ulong ocw, osw, otw, oip, ocs, ooo, oos, ost[20];
};

/*
 * Bits in CR0
 */
#define BIT(x) (1 << (x))
#define CR0_PE BIT(0)	/* Protection enable */
#define CR0_MP BIT(1)	/* Math processor present */
#define CR0_EM BIT(2)	/* Emulate FP--trap on FP instruction */
#define CR0_TS BIT(3)	/* Task switched flag */
#define CR0_ET BIT(4)	/* Extension type--387 DX presence */
#define CR0_NE BIT(5)	/* Numeric Error--allow traps on numeric errors */
#define CR0_WP BIT(16)	/* Write protect--ring 0 honors RO PTE's */
#define CR0_AM BIT(18)	/* Alignment--trap on unaligned refs */
#define CR0_NW BIT(29)	/* Not write-through--inhibit write-through */
#define CR0_CD BIT(30)	/* Cache disable */
#define CR0_PG BIT(31)	/* Paging--use PTEs/CR3 */

#ifdef KERNEL
extern void fpu_enable(struct fpu *), fpu_disable(struct fpu *),
	fpu_maskexcep(void);
extern int fpu_detected(void);
#endif /* KERNEL */

#endif /* _MACHREG_H */
@


1.3
log
@Reorder event frame so IP is in right place for use as target
of a ret instructino
@
text
@d80 29
@


1.2
log
@Add some defs for ptrace
@
text
@d15 3
a17 3
	ulong ev_prevsp;
	ulong ev_previp;
	char ev_event[EVLEN];
@


1.1
log
@Initial revision
@
text
@d35 1
d65 14
@
