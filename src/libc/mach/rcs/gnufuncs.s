head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@# @;


1.2
date	94.04.06.00.37.07;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.11.19.15.09;	author vandys;	state Exp;
branches;
next	;


desc
@Support routines for GCC-generated code
@


1.2
log
@Fix DOS \r\n brain damage
@
text
@/* History:105,12 */
/* gnu's CC1 library of math functions */

/*
**	12(%ebp) == b
**	8(%ebp) == a
*/

	.text

	.globl	___udivsi3
___udivsi3:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%eax
	movl	$0,%edx
	divl	12(%ebp)
	popl	%ebp
	ret

	.globl	___divsi3
___divsi3:
	pushl	%ebp
	movl	%esp,%ebp
	movl	8(%ebp),%eax
	cdq
	idivl	12(%ebp)
	popl	%ebp
	ret

	.globl	___fixdfsi
___fixdfsi:
	.globl	___fixunsdfsi
___fixunsdfsi:
	pushl	%ebp
	movl	%esp,%ebp
	sub	$12,%esp	/* -12 = old CW, -10 = new CW, -8 = result */
	fstcw	-12(%ebp)
	fwait
	movw	-12(%ebp),%ax
	orw	$0x0c00,%ax
	movw	%ax,-10(%ebp)
	fldcww	-10(%ebp)
	mov	8(%ebp),%eax	/* for debugging only */
	fwait
	fldl	8(%ebp)
	fwait
	fistpq	-8(%ebp)
	fwait
	fldcww	-12(%ebp)
	fwait
	mov	-8(%ebp),%eax
	movl	%ebp,%esp
	popl	%ebp
	ret
@


1.1
log
@Initial revision
@
text
@a55 1

@
