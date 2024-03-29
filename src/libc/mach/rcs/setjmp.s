head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@# @;


1.3
date	93.05.06.23.25.21;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.08.23.04.12;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.03.17;	author vandys;	state Exp;
branches;
next	;


desc
@setjmp/longjmp stuff, in assembly to access machine state
@


1.3
log
@Don't allow edi to change on setjmp(); breaks -O'ed code.
@
text
@/*
 * setjmp.s
 *	Routines for saving and restoring a context
 */

/*
 * From genassym in the kernel, but not worth adding such nonsense
 * here.
 */
R_EIP=0x00
R_EDI=0x04
R_ESI=0x08
R_EBP=0x0C
R_ESP=0x10
R_EBX=0x14
R_EDX=0x18
R_ECX=0x1C
R_EAX=0x20

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
	movl	R_EDI(%edi),%edi	/* Restore old edi value */
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
@


1.2
log
@Hard-code structure offsets; don't want to add a genassym to my
C library build.
@
text
@d39 1
@


1.1
log
@Initial revision
@
text
@d7 14
@
