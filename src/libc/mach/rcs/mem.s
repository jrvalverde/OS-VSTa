head	1.3;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@# @;


1.3
date	94.09.07.19.17.54;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.16.19.06.19;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.03.03;	author vandys;	state Exp;
branches;
next	;


desc
@Block memory routines in assembly language
@


1.3
log
@Update bzero()/bcopy() to faster kernel versions, set up so
same routines can be shared by kernel and libc.
@
text
@/*
 * mem.s
 *	Memory operations worth coding in assembler for speed
 */

/*
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
@


1.2
log
@Add getpagesize()
@
text
@d16 4
d21 6
d28 1
a28 1
	popl	%ecx
d43 2
a44 1
	cmpl	%edi,%esi
d46 13
a58 1
	rep		/* No ripple in forward copy */
d61 1
a61 1
3:	popl	%ecx	/* Restore registers and return */
d66 7
a72 1
2:			/* Potential ripple in forward; copy backwards */
d74 1
a74 1
	addl	%ecx,%esi
a81 10

/*
 * getpagesize()
 *	Easy to answer here, eh?
 */
	.text
	.globl	_getpagesize
_getpagesize:
	movl	$0x1000,%eax
	ret
@


1.1
log
@Initial revision
@
text
@d53 10
@
