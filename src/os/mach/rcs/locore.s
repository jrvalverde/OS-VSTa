head	1.18;
access;
symbols
	V1_3_1:1.11
	V1_3:1.11
	V1_2:1.11
	V1_1:1.9
	V1_0:1.9;
locks; strict;
comment	@# @;


1.18
date	94.12.28.21.52.04;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.12.21.05.26.56;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.11.16.19.35.14;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.10.06.01.44.59;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.10.05.17.57.46;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.09.07.19.17.08;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.08.30.21.56.14;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.12.22.00.21.26;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.12.09.06.17.22;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.07.07.00.53.07;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.07.07.00.31.46;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.06.30.19.55.59;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.06.27.17.39.22;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.05.06.23.26.55;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.23.22.42.51;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.19.21.42.20;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.12.19.44.52;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.05.47;	author vandys;	state Exp;
branches;
next	;


desc
@Stuff which really needs to be in assembly
@


1.18
log
@Add halt() assembly support so we can use CPU halt function to
preserve heat and power.
@
text
@/*
 * locore.s
 *	Assembly support for VSTa
 */
#include "assym.h"

/* Current thread executing */
	.globl	_cpu
#define CURTHREAD (_cpu+PC_THREAD)
#define SYNC	inb $0x84,%al

	.data
	.globl	_free_pfn,_size_ext,_size_base,_boot_pfn
_free_pfn: .space	4	/* PFN of first free page beyond data */
_size_base: .space	4	/* # pages of base (< 640K) memory */
_size_ext:  .space	4	/*  ... extended (> 1M) memory */
_boot_pfn:  .space	4	/* PFN of first boot task */
mainretmsg:
	.asciz	"main returned"

/*
 * Entered through 32-bit task gate constructed in 16-bit mode
 *
 * Our parameters are passed on the stack, which is located
 * in high memory.  Before any significant memory use occurs,
 * we must switch it down to a proper stack.
 */
	.text
	.globl	_start,_main
_start:
#define GETP(var) popl %eax ; movl %eax,_##var
	GETP(free_pfn)
	GETP(size_ext)
	GETP(size_base)
	GETP(boot_pfn);
#undef GETP

	/*
	 * Call our first C code on the idle stack
	 */
	movl	$_id_stack,%esp
	movl	$_id_stack,%ebp
	call	_main
1:
	.globl	_panic
	pushl	$mainretmsg
	call	_panic
	jmp	1b

/*
 * cli()
 *	Disable interrupt, return old status
 *
 * Returns non-zero if they were enabled before, otherwise 0
 */
	.globl	_cli
_cli:	pushfl
	cli
	popl	%eax
	andl	$0x200,%eax
	ret

/*
 * sti()
 *	Enable interrupts
 */
	.globl	_sti
_sti:	sti
	ret

/*
 * atomic inc/dec
 *	Lock prefix probably not even needed for uP
 */
	.globl	_ATOMIC_INC,_ATOMIC_DEC,_ATOMIC_INCL,_ATOMIC_DECL
_ATOMIC_INC:
_ATOMIC_INCL:
	movl	4(%esp),%eax
	lock
	incl	(%eax)
	ret
_ATOMIC_DEC:
_ATOMIC_DECL:
	movl	4(%esp),%eax
	lock
	decl	(%eax)
	ret
	.globl	_ATOMIC_INCW,_ATOMIC_DECW
_ATOMIC_INCW:
	movl	4(%esp),%eax
	lock
	incw	(%eax)
	ret
_ATOMIC_DECW:
	movl	4(%esp),%eax
	lock
	decw	(%eax)
	ret

/*
 * get_cr3()/set_cr3()/get_cr2()/get_cr0()/set_cr0()
 *	Return/set value of root page table register and fault addr
 */
	.globl	_get_cr3,_set_cr3,_get_cr2,_get_cr0,_set_cr0
_get_cr3:
	movl	%cr3,%eax
	ret
_get_cr2:
	movl	%cr2,%eax
	ret
_set_cr3:
	SYNC
	movl	4(%esp),%eax
	movl	%eax,%cr3
	SYNC
	movl	%cr3,%eax
	ret
_get_cr0:
	movl	%cr0,%eax
	ret
_set_cr0:
	movl	4(%esp),%eax
	movl	%eax,%cr0
	movl	%cr0,%eax
	ret

/*
 * lgdt()/lidt()
 *	Load GDT/IDT registers
 */
	.globl	_lgdt,_lidt
_lgdt:	movl	4(%esp),%eax
	nop
	lgdt	(%eax)
	jmp	1f
1:	nop
	ret
_lidt:	movl	4(%esp),%eax
	lidt	(%eax)
	ret

/*
 * ltr()
 *	Load task register
 */
	.globl	_ltr
_ltr:	ltr	4(%esp)
	ret

/*
 * flush_tlb()
 *	Flush whole TLB.
 *
 * i486 has individual flush, but probably not worth the bother.
 */
	.globl	_flush_tlb
_flush_tlb:
	SYNC
	movl	%cr3,%eax
	movl	%eax,%cr3
	SYNC
1:	ret

/*
 * setjmp()
 *	Save context, return 0
 */
	.globl	_setjmp
_setjmp:
	pushl	%edi
	movl	8(%esp),%edi		/* jmp_buf pointer */
	movl	4(%esp),%eax		/* saved eip */
	movl	%eax,R_EIP(%edi)
	popl	%eax			/* edi's original value */
	movl	%eax,R_EDI(%edi)
	movl	%esi,R_ESI(%edi)	/* esi,ebp,esp,ebx,edx,ecx,eax */
	movl	%ebp,R_EBP(%edi)
	movl	%esp,R_ESP(%edi)
	movl	%ebx,R_EBX(%edi)
	movl	%edx,R_EDX(%edi)
	movl	%ecx,R_ECX(%edi)
/*	movl	%eax,R_EAX(%edi) Why bother? */
	movl	R_EDI(%edi),%edi	/* restore old edi */
	xorl	%eax,%eax
	ret

/*
 * longjmp()
 *	Restore context, return second argument as value
 */
	.globl	_longjmp
_longjmp:
	movl	4(%esp),%edi		/* jmp_buf */
	movl	8(%esp),%eax		/* return value (eax) */
	movl	%eax,R_EAX(%edi)
	movl	R_ESP(%edi),%esp	/* switch to new stack position */
	movl	R_EIP(%edi),%eax	/* get new ip value */
	movl	%eax,(%esp)
	movl	R_ESI(%edi),%esi	/* esi,ebp,ebx,edc,ecx,eax */
	movl	R_EBP(%edi),%ebp
	movl	R_EBX(%edi),%ebx
	movl	R_EDX(%edi),%edx
	movl	R_ECX(%edi),%ecx
	movl	R_EAX(%edi),%eax
	movl	R_EDI(%edi),%edi	/* get edi last */
	ret

/*
 * fpu_disable()
 *	Disable FPU access, save current state if have pointer
 */
	.globl	_fpu_disable
_fpu_disable:
	movl	4(%esp),%eax
	orl	%eax,%eax
	jz	1f
	fnsave	(%eax)
1:	movl	%cr0,%eax
	orl	$(CR0_EM),%eax
	movl	%eax,%cr0
	ret

/*
 * fpu_enable()
 *	Enable FPU access, load current state if have pointer
 */
	.globl	_fpu_enable
_fpu_enable:
	movl	%cr0,%eax	/* Turn on access */
	andl	$~(CR0_EM|CR0_TS),%eax
	movl	%eax,%cr0
	movl	4(%esp),%eax
	orl	%eax,%eax
	jz	_fpu_init	/* No FPU state--init FPU instead */
	frstor	(%eax)		/* Load old FPU state */
	ret

/*
 * fpu_init()
 *	Clear FPU state to its basic form
 */
	.globl	_fpu_init
_fpu_init:
	fnclex
	fninit
	ret

/*
 * fpu_detected()
 *	Tell if an FPU is present on this CPU
 *
 * Note, if you have an i287 on your system, you deserve everything
 * you're about to get.
 */
	.globl	_fpu_detected
_fpu_detected:
	fninit
	fstsw	%ax		/* See if an FP operation happens */
	orb	%al,%al
	je	1f
	xorl	%eax,%eax	/* Nope */
	ret
1:	xorl	%eax,%eax	/* Yup */
	incl	%eax
	ret

/*
 * fpu_maskexcep()
 *	Mask out pending exceptions
 */
	.globl	_fpu_maskexcep
_fpu_maskexcep:
	leal	-4(%esp),%esp		/* Put ctl word on stack */
	fnstcw	(%esp)
	fnstsw	%ax			/* Status word -> AX */
	andw	$0x3f,%ax		/* Clear pending exceps */
	orw	%ax,(%esp)
	fnclex
	fldcw	(%esp)			/* Load new ctl word */
	leal	4(%esp),%esp		/* Free temp storage */
	ret

/*
 * inportb()
 *	Get a byte from an I/O port
 */
	.globl	_inportb
_inportb:
	movl	4(%esp),%edx
	xorl	%eax,%eax
	SYNC ; SYNC
	inb	%dx,%al
	ret

/*
 * outportb()
 *	Write a byte to an I/O port
 */
	.globl	_outportb
_outportb:
	movl	4(%esp),%edx
	SYNC ; SYNC
	movl	8(%esp),%eax
	outb	%al,%dx
	SYNC ; SYNC
	ret

/*
 * Common body for setting up a copyin/out
 */
#define CP_PROLOG \
	/* Validate base address and range */  ; \
	movl	0x4(%esp),%eax		/* In 1st half */  ; \
	cmpl	$0x80000000,%eax ; \
	jae	bad_copy  ; \
	movl	0xC(%esp),%eax  ; \
	cmpl	$0x40000000,%eax	/* Catch overflow tricks */  ; \
	jae	bad_copy  ; \
	addl	0x4(%esp),%eax		/* End in 1st half also */  ; \
	cmpl	$0x80000000,%eax ; \
	jae	bad_copy  ; \
  \
	/* Save registers */  ; \
	pushl	%edi  ; \
	pushl	%esi  ; \
  \
	/* Flag probe */  ; \
	movl	CURTHREAD,%edi  ; \
	movl	$_cpfail,T_PROBE(%edi)

/*
 * Common body for moving data in copyin/out
 */
#define CP_MOVE \
	/* Try to move longwords */ ; \
	shrl	$2,%ecx ; \
	cld ; rep ; \
	movsl ; \
 \
	/* Move residual bytes */ ; \
	movl	0x14(%esp),%ecx ; \
	andl	$3,%ecx ; \
	cld ; rep ; \
	movsb

/*
 * Common body for cleaning up after copyin/out
 */
#define CP_POSTLOG \
	/* Clear probe */ ; \
	movl	CURTHREAD,%edi ; \
	xorl	%eax,%eax ; \
	movl	%eax,T_PROBE(%edi) ; \
  \
	/* Restore registers used */ ; \
	popl	%esi ; \
	popl	%edi

/*
 * copyin(uaddr, sysaddr, nbyte)
 *	Copy in bytes from user to kernel
 */
	.globl	_copyin
_copyin:
	CP_PROLOG

	/* Load source, dest, count */
	movl	0x10(%esp),%edi
	movl	0x0C(%esp),%esi
	addl	$0x80000000,%esi
	movl	0x14(%esp),%ecx

	CP_MOVE

	CP_POSTLOG

	ret

/*
 * cpfail()
 *	Entered from fault handler on failed page fault with t_probe
 *
 * t_probe points here.  We need to restore our registers and then
 * return an error.
 */
	.globl	_cpfail
_cpfail:
	popl	%esi
	popl	%edi

	/* VVV fall into VVV */
	
/* Return error for copyin/copyout */
bad_copy:
	movl	$-1,%eax
	ret

/*
 * copyout(uaddr, sysaddr, nbyte)
 *	Copy bytes out to user from kernel
 */
	.globl	_copyout
_copyout:
	CP_PROLOG

	/* Load source, dest, count */
	movl	0xC(%esp),%edi
	addl	$0x80000000,%edi
	movl	0x10(%esp),%esi
	movl	0x14(%esp),%ecx

	CP_MOVE

	CP_POSTLOG

	ret

/*
 * uucopy(uaddr1, uaddr2, nbyte)
 *	Copy bytes user->user
 *
 * This is used to copy memory from a message reply after attaching
 * it to the user's address space.
 *
 * We just map the "from" address into user space, then let copyout()
 * do all the work.  This means we trust "from", but since it's a
 * system-generated address, this should be OK.
 */
	.globl	_uucopy
_uucopy:
	orl	$0x80000000,8(%esp)
	jmp	_copyout

/*
 * nop()
 *	No-op, to trick the compiler
 */
	.globl	_nop
_nop:	ret

/*
 * reload_dr()
 *	Load db0..3 and db7 from the "struct dbg_regs" in machreg.h
 */
	.globl	_reload_dr
_reload_dr:
	pushl	%ebx
	movl	8(%esp),%ebx
	movl	0(%ebx),%eax
	movl	%eax,%db0
	movl	4(%ebx),%eax
	movl	%eax,%db1
	movl	8(%ebx),%eax
	movl	%eax,%db2
	movl	0xC(%ebx),%eax
	movl	%eax,%db3
	movl	0x10(%ebx),%eax
	movl	%eax,%db7
	popl	%ebx
	ret

/*
 * idle_stack()
 *	Switch to an idle stack
 *
 * The stack is empty; don't try to return to your previous stack frame!
 */
	.data
	.space	0x1000
_id_stack:
	.space	0x40		/* Slop */
	.globl	_id_stack
	.text
_idle_stack:
	.globl	_idle_stack
	popl	%eax
	movl	$_id_stack,%esp
	movl	$_id_stack,%ebp
	jmp	%eax

/*
 * idle()
 *	Idle waiting for work
 *
 * We watch for num_run to go non-zero; we use sti/halt to atomically
 * enable interrupts and halt the CPU--this saves a fair amount of power
 * and heat.
 */
	.globl	_idle,_num_run
_idle:	cli
	movl	_num_run,%eax
	orl	%eax,%eax
	jnz	1f
	sti
	hlt
	jmp	_idle
1:	sti
	ret

/*
 * Common macros to force segment registers to appropriate value
 */
#define PUSH_SEGS pushw %ds ; pushw %es
#define POP_SEGS popw %es ; popw %ds
#define SET_KSEGS movw $(GDT_KDATA),%ax ; movw %ax,%ds ; movw %ax,%es
#define SET_USEGS movw $(GDT_UDATA),%ax ; movw %ax,%ds ; movw %ax,%es

/*
 * trap_common()
 *	Common code for all traps
 *
 * At this point all the various traps and exceptions have been moulded
 * into a single stack format--a OS-type trap number, an error code (0
 * for those which don't have one), and then the saved registers followed
 * by a trap-type stack frame suitable for iret'ing.
 */
trap_common:
	/* Save all user's registers */
	pushal
	cld
	PUSH_SEGS

	/* Switch to kernel data segments */
	SET_KSEGS

	/* Call C-code for trap() */
	call	_trap

	/* May as well share the code for this... */
	.globl	_retuser
_retuser:

	POP_SEGS

	/* Get registers back, drop OS trap type and error number */
	popal
	addl	$8,%esp

	/* Back to where we came from... */
	iret

/*
 * stray_intr()
 *	Handling of any vectors for which we have no handler
 */
	.text
stray_msg:
	.asciz	"stray interrupt"

	.globl	_stray_intr
_stray_intr:
	pushal
	SET_KSEGS
	pushl	$stray_msg
1:	call	_panic
	jmp	1b

/*
 * intr_common()
 *	Common code for all interrupts
 */
	.globl	intr_common,_interrupt
intr_common:
	pushal
	cld
	PUSH_SEGS
	SET_KSEGS
	call	_interrupt
	POP_SEGS
	popal
	addl	$8,%esp
	iret

/*
 * Templates for entry handling.  IDTERR() is for entries which
 * already have an error code supplied by the i386.  IDT() is for
 * those which don't--we push a dummy 0.
 */
#define IDT(n, t) 	.globl _##n ; _##n##: \
	pushl $0 ; pushl $(t) ; jmp trap_common
#define IDTERR(n, t) 	.globl _##n ; _##n##: \
	pushl $(t) ; jmp trap_common

/*
 * The vectors we handle
 */
IDT(Tdiv, T_DIV)
IDT(Tdebug, T_DEBUG)
IDT(Tnmi, T_NMI)
IDT(Tbpt, T_BPT)
IDT(Tovfl, T_OVFL)
IDT(Tbound, T_BOUND)
IDT(Tinstr, T_INSTR)
IDT(T387, T_387)
IDTERR(Tdfault, T_DFAULT)
IDTERR(Tcpsover, T_CPSOVER)
IDTERR(Tinvtss, T_INVTSS)
IDTERR(Tseg, T_SEG)
IDTERR(Tstack, T_STACK)
IDTERR(Tgenpro, T_GENPRO)
IDTERR(Tpgflt, T_PGFLT)
IDT(Tnpx, T_NPX)
IDT(Tsyscall, T_SYSCALL)

/*
 * stray_ign()
 *	A fast path for ignoring known stray interrupts
 *
 * Currently used for IRQ7, even when lpt1 is masked on the PIC!
 */
	.globl	_stray_ign
_stray_ign:
	iret

/*
 * INTVEC()
 *	Macro to set up trap frame for hardware interrupt
 *
 * We waste the extra pushl to make it look much like a trap frame.
 * If the extra cycle/write is a problem, guess we could define
 * a "struct intframe" in machreg.h.
 */
#define INTVEC(n)	.globl	_xint##n ; _xint##n##: \
	pushl	$(0); pushl	$(n) ; jmp intr_common

	.align	4
INTVEC(32)
INTVEC(33)
INTVEC(34)
INTVEC(35)
INTVEC(36)
INTVEC(37)
INTVEC(38)
INTVEC(39)
INTVEC(40)
INTVEC(41)
INTVEC(42)
INTVEC(43)
INTVEC(44)
INTVEC(45)
INTVEC(46)
INTVEC(47)
INTVEC(48)
INTVEC(49)
INTVEC(50)
INTVEC(51)
INTVEC(52)
INTVEC(53)
INTVEC(54)
INTVEC(55)
INTVEC(56)
INTVEC(57)
INTVEC(58)
INTVEC(59)
INTVEC(60)
INTVEC(61)
INTVEC(62)
INTVEC(63)
INTVEC(64)
INTVEC(65)
INTVEC(66)
INTVEC(67)
INTVEC(68)
INTVEC(69)
INTVEC(70)
INTVEC(71)
INTVEC(72)
INTVEC(73)
INTVEC(74)
INTVEC(75)
INTVEC(76)
INTVEC(77)
INTVEC(78)
INTVEC(79)
INTVEC(80)
INTVEC(81)
INTVEC(82)
INTVEC(83)
INTVEC(84)
INTVEC(85)
INTVEC(86)
INTVEC(87)
INTVEC(88)
INTVEC(89)
INTVEC(90)
INTVEC(91)
INTVEC(92)
INTVEC(93)
INTVEC(94)
INTVEC(95)
INTVEC(96)
INTVEC(97)
INTVEC(98)
INTVEC(99)
INTVEC(100)
INTVEC(101)
INTVEC(102)
INTVEC(103)
INTVEC(104)
INTVEC(105)
INTVEC(106)
INTVEC(107)
INTVEC(108)
INTVEC(109)
INTVEC(110)
INTVEC(111)
INTVEC(112)
INTVEC(113)
INTVEC(114)
INTVEC(115)
INTVEC(116)
INTVEC(117)
INTVEC(118)
INTVEC(119)
INTVEC(120)
INTVEC(121)
INTVEC(122)
INTVEC(123)
INTVEC(124)
INTVEC(125)
INTVEC(126)
INTVEC(127)
INTVEC(128)
INTVEC(129)
INTVEC(130)
INTVEC(131)
INTVEC(132)
INTVEC(133)
INTVEC(134)
INTVEC(135)
INTVEC(136)
INTVEC(137)
INTVEC(138)
INTVEC(139)
INTVEC(140)
INTVEC(141)
INTVEC(142)
INTVEC(143)
INTVEC(144)
INTVEC(145)
INTVEC(146)
INTVEC(147)
INTVEC(148)
INTVEC(149)
INTVEC(150)
INTVEC(151)
INTVEC(152)
INTVEC(153)
INTVEC(154)
INTVEC(155)
INTVEC(156)
INTVEC(157)
INTVEC(158)
INTVEC(159)
INTVEC(160)
INTVEC(161)
INTVEC(162)
INTVEC(163)
INTVEC(164)
INTVEC(165)
INTVEC(166)
INTVEC(167)
INTVEC(168)
INTVEC(169)
INTVEC(170)
INTVEC(171)
INTVEC(172)
INTVEC(173)
INTVEC(174)
INTVEC(175)
INTVEC(176)
INTVEC(177)
INTVEC(178)
INTVEC(179)
INTVEC(180)
INTVEC(181)
INTVEC(182)
INTVEC(183)
INTVEC(184)
INTVEC(185)
INTVEC(186)
INTVEC(187)
INTVEC(188)
INTVEC(189)
INTVEC(190)
INTVEC(191)
INTVEC(192)
INTVEC(193)
INTVEC(194)
INTVEC(195)
INTVEC(196)
INTVEC(197)
INTVEC(198)
INTVEC(199)
INTVEC(200)
INTVEC(201)
INTVEC(202)
INTVEC(203)
INTVEC(204)
INTVEC(205)
INTVEC(206)
INTVEC(207)
INTVEC(208)
INTVEC(209)
INTVEC(210)
INTVEC(211)
INTVEC(212)
INTVEC(213)
INTVEC(214)
INTVEC(215)
INTVEC(216)
INTVEC(217)
INTVEC(218)
INTVEC(219)
INTVEC(220)
INTVEC(221)
INTVEC(222)
INTVEC(223)
INTVEC(224)
INTVEC(225)
INTVEC(226)
INTVEC(227)
INTVEC(228)
INTVEC(229)
INTVEC(230)
INTVEC(231)
INTVEC(232)
INTVEC(233)
INTVEC(234)
INTVEC(235)
INTVEC(236)
INTVEC(237)
INTVEC(238)
INTVEC(239)
INTVEC(240)
INTVEC(241)
INTVEC(242)
INTVEC(243)
INTVEC(244)
INTVEC(245)
INTVEC(246)
INTVEC(247)
INTVEC(248)
INTVEC(249)
INTVEC(250)
INTVEC(251)
INTVEC(252)
INTVEC(253)
INTVEC(254)
@


1.17
log
@Omit unused interrupt vector
@
text
@d482 19
@


1.16
log
@Add long/int/short atomic ops
@
text
@a830 1
INTVEC(255)
@


1.15
log
@Fix gas 2.X syntax
@
text
@d75 1
a75 1
	.globl	_ATOMIC_INC,_ATOMIC_DEC
d77 1
d83 1
d87 11
@


1.14
log
@Add FPU support
@
text
@d206 1
a206 1
	orl	$CR0_EM,%eax
@


1.13
log
@Move bzero()/bcopy() to shared point in the libraries
@
text
@d88 1
a88 1
 * get_cr3()/set_cr3()/get_cr2()
d91 1
a91 1
	.globl	_get_cr3,_set_cr3,_get_cr2
d105 8
d196 75
d563 2
a564 1
IDT(Tdfault, T_DFAULT)
@


1.12
log
@Need to deref pointer to curthread
@
text
@a71 77
 * bzero()
 *	Zero some memory
 */
	.globl	_bzero
_bzero:	pushl	%edi
	pushl	%ecx
	movl	0xC(%esp),%edi
	movl	0x10(%esp),%ecx
	xorl	%eax,%eax
	cmpl	$4,%ecx			/* Zero longwords */
	jb	1f
	shrl	$2,%ecx			/* Scale to longword count */
	cld
	rep
	stosl
	movl	0x10(%esp),%ecx		/* Calculate byte resid */
	andl	$3,%ecx
	jz	2f
1:	cld
	rep				/* Zero bytes */
	stosb
2:	popl	%ecx
	popl	%edi
	ret

/*
 * bcopy()
 *	Copy some memory
 */
	.globl	_bcopy
_bcopy:	pushl	%esi
	pushl	%edi
	pushl	%ecx
	movl	0x10(%esp),%esi
	movl	0x14(%esp),%edi
	movl	0x18(%esp),%ecx

	cmpl	%edi,%esi	/* Potential ripple in copy? */
	jb	2f

1:	cmpl	$4,%ecx		/* Move longwords */
	jb	5f
	shrl	$2,%ecx		/* Scale units */
	cld
	rep
	movsl
	movl	0x18(%esp),%ecx
	andl	$3,%ecx
	jz	3f

5:	cld
	rep			/* Resid copy of bytes */
	movsb

3:	popl	%ecx		/* Restore registers and return */
	popl	%edi
	popl	%esi
	ret

2:	addl	%ecx,%esi	/* If no overlap, still copy forward */
	cmpl	%edi,%esi
	jae	4f
	movl	0x10(%esp),%esi	/* Restore register */
	jmp	1b		/* Forward copy */

4:				/* Overlap; copy backwards */
	std
	/* addl	%ecx,%esi	Done in overlap check */
	decl	%esi
	addl	%ecx,%edi
	decl	%edi
	rep
	movsb
	cld
	jmp	3b

/*
@


1.11
log
@Tidy up spacing, use special pasting to be more portable
across various C preprocessors
@
text
@d309 1
a309 1
	movl	$(CURTHREAD),%edi  ; \
d332 1
a332 1
	movl	$(CURTHREAD),%edi ; \
@


1.10
log
@Add ptrace() support
@
text
@d19 1
a19 1
		.asciz	"main returned"
d541 1
a541 1
#define IDT(n, t) 	.globl _##n ; _##n: \
d543 1
a543 1
#define IDTERR(n, t) 	.globl _##n ; _##n: \
d584 1
a584 1
#define INTVEC(n)	.globl	_xint##n ; _xint##n: \
@


1.9
log
@Don't trust state of D bit
@
text
@d423 21
@


1.8
log
@Always set D bit to known state on entry to the kernel
@
text
@d84 1
d90 2
a91 1
1:	rep				/* Zero bytes */
d115 1
d122 2
a123 1
5:	rep			/* Resid copy of bytes */
d318 1
a318 1
	rep ; \
d324 1
a324 1
	rep ; \
@


1.7
log
@Fix macro parms for GCC 2.X.  Add inb interlocks to fix
race with pipelining of 486-class CPU.
@
text
@d457 1
d502 1
@


1.6
log
@Get rid of test code
@
text
@d10 1
d172 1
d175 1
d210 1
a211 1
	nop
d213 2
a214 3
	jmp	1f
1:	nop
	ret
d268 1
d279 1
d282 1
d305 1
a305 1
	movl	$CURTHREAD,%edi  ; \
d328 1
a328 1
	movl	$CURTHREAD,%edi ; \
d442 2
a443 2
#define SET_KSEGS movw $GDT_KDATA,%ax ; movw %ax,%ds ; movw %ax,%es
#define SET_USEGS movw $GDT_UDATA,%ax ; movw %ax,%ds ; movw %ax,%es
d515 1
a515 1
	pushl $0 ; pushl $t ; jmp trap_common
d517 1
a517 1
	pushl $t ; jmp trap_common
d558 1
a558 1
	pushl	$0; pushl	$n ; jmp intr_common
@


1.5
log
@Don't allow %edi to change on setjmp(); broke in C library, but
is wrong here as well.
@
text
@a185 2
	movl	$GDT_KDATA,%ax	/* XXX test */
	movw	%ax,%gs
@


1.4
log
@Implement KDB
@
text
@d235 1
@


1.3
log
@Use longword-moves when possible in bcopy/bzero
@
text
@d17 2
d28 1
a28 1
	.globl	_start,_main,_dbg_enter
d44 3
a46 1
	call	_dbg_enter
@


1.2
log
@Add fast path ignore for known stray interrupts
@
text
@d76 3
d80 5
d86 1
a86 1
	popl	%ecx
d101 2
a102 1
	cmpl	%edi,%esi
d104 11
a114 1
	rep		/* No ripple in forward copy */
d117 1
a117 1
3:	popl	%ecx	/* Restore registers and return */
d122 7
a128 1
2:			/* Potential ripple in forward; copy backwards */
d130 1
a130 1
	addl	%ecx,%esi
@


1.1
log
@Initial revision
@
text
@d507 10
@
