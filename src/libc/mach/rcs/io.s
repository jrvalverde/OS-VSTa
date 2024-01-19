head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@# @;


1.4
date	94.08.27.00.13.44;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.06.00.37.07;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.15.35.11;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.02.55;	author vandys;	state Exp;
branches;
next	;


desc
@inbyte/outbyte and such routines
@


1.4
log
@Add in/out of words
@
text
@/*
 * io.s
 *	Routines for accessing i386 I/O ports
 */

/*
 * inportb()
 *	Get a byte from an I/O port
 */
	.globl	_inportb
_inportb:
	movl	4(%esp),%edx
	xorl	%eax,%eax
	inb	%dx,%al
	ret

/*
 * outportb()
 *	Write a byte to an I/O port
 */
	.globl	_outportb
_outportb:
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	outb	%al,%dx
	ret
/*
 * inportw()
 *	Get a word from an I/O port
 */
	.globl	_inportw
_inportw:
	movl	4(%esp),%edx
	xorl	%eax,%eax
	inw	%dx,%ax
	ret

/*
 * outportw()
 *	Write a word to an I/O port
 */
	.globl	_outportw
_outportw:
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	outw	%ax,%dx
	ret

/*
 * repinsw(port, buffer, count)
 *	Read a bunch of words from an I/O port
 */
	.globl	_repinsw
_repinsw:
	pushl	%edi
	movl	0x8(%esp),%edx
	movl	0xC(%esp),%edi
	movl	0x10(%esp),%ecx
	rep
	insw
	popl	%edi
	ret

/*
 * repoutsw(port, buffer, count)
 *	Write a bunch of words to an I/O port
 */
	.globl	_repoutsw
_repoutsw:
	pushl	%esi
	movl	0x8(%esp),%edx
	movl	0xC(%esp),%esi
	movl	0x10(%esp),%ecx
	rep
	outsw
	popl	%esi
	ret
@


1.3
log
@Fix DOS \r\n brain damage
@
text
@d27 21
@


1.2
log
@Add block in/out support for disk driver
@
text
@a56 1

@


1.1
log
@Initial revision
@
text
@d27 31
@
