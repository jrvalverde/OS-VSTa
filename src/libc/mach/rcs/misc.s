head	1.1;
access;
symbols;
locks; strict;
comment	@# @;


1.1
date	94.09.07.19.17.29;	author vandys;	state Exp;
branches;
next	;


desc
@Move getpagesize() to misc file; bzero()/bcopy() are imported
by kernel, which doesn't need this function.
@


1.1
log
@Initial revision
@
text
@/*
 * getpagesize()
 *	Easy to answer here, eh?
 */
	.text
	.globl	_getpagesize
_getpagesize:
	movl	$0x1000,%eax
	ret
@
