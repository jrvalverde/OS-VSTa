head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@# @;


1.4
date	94.09.30.22.53.30;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.07.09.18.36.04;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.06.30.19.53.56;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.02.44;	author vandys;	state Exp;
branches;
next	;


desc
@C runtime assembly code
@


1.4
log
@Add setup of iob[] and ctab[] for user programs.  start.c does
the same for the shlib side of the world.
@
text
@/*
 * crt0.s
 *	C Run-Time code at 0
 *
 * Not really at 0 any more, but the name sticks.  Interestingly, the
 * orginal magic numbers (407, 411, 413, etc.--octal, you understand)
 * were PDP-11 jump instructions.  Their presence at location 0 not
 * only allowed the exec() system call to determine the type of the
 * executable; the a.out was always run at 0 and the jump would then
 * skip over the rest of the header in memory and start at the right
 * place.  Since 413 was a demand-paged format, and the PDP-11 never
 * got pages, this likely doesn't hold true for this value.
 *
 * Anyway.  For us, we need to point to the well-known location for
 * the argv array, and pull its length onto the stack as well.  Then
 * we simply fire up the user's code.
 */
	.data
argc:	.long	0	/* Private variables for __start() to fill in */
argv:	.long	0
	.globl	___iob,___ctab
___iob:	.long	0	/* Data "shared" with libc users */
___ctab: .long	0
	.text

	.globl	start,_main,_exit
start:

/*
 * Special code for boot servers.  This is pretty ugly, but the benefits
 * are large.  We put a static data area at the front of boot server
 * a.out's, so that the boot loader program boot.exe can fill this
 * area with an initial argument string, argc, and argv.
 */
#ifdef SRV

/* These are made available by putting them into an initialized area */
#define MAXNARG 8	/* Max # arguments */
#define MAXARG 64	/* Max # bytes of argument string */

	.globl	___bootargc,___bootargv,___bootarg
	jmp	2f		/* Skip this data area */
	nop ; nop		/* For future expansion */
	nop ; nop ; nop ; nop
___bootargc:
	.long	0		/* argc */
___bootargv:
	.long	0xDEADBEEF
	.word	MAXNARG,MAXARG
	.space	((MAXNARG-2)*4)	/* argv */
___bootarg:
	.space	MAXARG		/*  ...rest of arg space */
2:
#endif /* SRV */

	pushl	$argv
	pushl	$argc
#ifdef SRV
	.globl	___start2
	call	___start2	/* Need a special version to understand */
#else				/*  the boot string rea */
	.globl	___start
	call	___start
#endif
	addl	$8,%esp
	call	___get_iob	/* Get iob and ctab array values */
	movl	%eax,___iob
	call	___get_ctab
	movl	%eax,___ctab
	pushl	argv		/* Call main(argc, argv) */
	pushl	argc
	call	_main
	addl	$8,%esp
	pushl	%eax
1:	call	_exit		/* exit() with value returned from main() */
	movl	$0,(%esp)
	jmp	1b

/* Dummy for GCC 2.X hooks into C++ */
	.globl	___main
___main:
	ret
@


1.3
log
@Add room to CRT0 to store boot arguments.  We compile a special
version of crt0.o (crt0srv.o) which makes this space available
to the boo loader.
@
text
@d21 3
d66 5
a70 1
	pushl	argv
d75 1
a75 1
1:	call	_exit
@


1.2
log
@Add dummy for GCC 2.X hook
@
text
@d23 1
a23 1
	.globl	start,___start,_main,_exit
d25 28
d55 5
d61 1
@


1.1
log
@Initial revision
@
text
@d37 5
@
