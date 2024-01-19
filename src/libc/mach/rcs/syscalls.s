head	1.31;
access;
symbols
	V1_3_1:1.24
	V1_3:1.24
	V1_2:1.19
	V1_1:1.18
	V1_0:1.16;
locks; strict;
comment	@# @;


1.31
date	94.12.21.05.34.57;	author vandys;	state Exp;
branches;
next	1.30;

1.30
date	94.10.04.20.27.34;	author vandys;	state Exp;
branches;
next	1.29;

1.29
date	94.10.04.19.49.10;	author vandys;	state Exp;
branches;
next	1.28;

1.28
date	94.09.27.21.32.38;	author vandys;	state Exp;
branches;
next	1.27;

1.27
date	94.08.29.01.43.25;	author vandys;	state Exp;
branches;
next	1.26;

1.26
date	94.08.25.00.57.26;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	94.07.06.04.44.26;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	94.04.07.00.11.17;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	94.04.06.00.32.00;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	94.03.15.22.05.34;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	94.02.27.02.29.43;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	94.02.01.23.22.16;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	93.12.09.06.18.53;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.11.16.02.50.35;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.10.01.19.07.12;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.06.30.19.54.10;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.04.20.21.24.55;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.04.12.20.54.51;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.03.30.01.09.22;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.03.26.23.38.29;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.03.26.23.31.45;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.03.24.19.08.56;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.03.20.00.21.37;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.03.03.23.15.33;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.02.26.18.41.31;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.02.23.18.19.19;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.19.15.35.25;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.08.15.08.04;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.03.20.13.09;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.01.15.46.36;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.03.33;	author vandys;	state Exp;
branches;
next	;


desc
@Interface functions to trap requests out to the kernel
@


1.31
log
@Add sched_op() syscall
@
text
@/*
 * syscalls.c
 *	Actual entry points into the VSTa operating system
 *
 * We keep the return address in a register so that tfork() will
 * return correctly even though the child thread returns on a new
 * stack.
 */
#include <sys/syscall.h>

/*
 * syserr()
 *	A hidden wrapper for system error string handling
 *
 * The point of this routine is NOT to get the error string.  We simply
 * ensure that any pending __errno changes are invalidated by the new
 * error status.  We also make sure that a flag is set to tell the libc
 * code that it should pick it's next error string up from the kernel
 */
	.data
	.globl	__errno
	.globl	__old_errno
	.globl	__err_sync
	.text

syserr:	movl	$1, __err_sync
	push	%eax
	movl	__errno, %eax
	movl	%eax, __old_errno
	pop	%eax
	ret

#define ENTRY(n, v)	.globl	_##n ; \
	_##n##: movl $(v),%eax ; int $0xFF ; jc syserr; ret

ENTRY(msg_port, S_MSG_PORT)
ENTRY(msg_connect, S_MSG_CONNECT)
ENTRY(msg_accept, S_MSG_ACCEPT)
ENTRY(msg_send, S_MSG_SEND)
ENTRY(msg_receive, S_MSG_RECEIVE)
ENTRY(msg_reply, S_MSG_REPLY)
ENTRY(msg_disconnect, S_MSG_DISCONNECT)
ENTRY(_msg_err, S_MSG_ERR)
ENTRY(_exit, S_EXIT)
ENTRY(fork, S_FORK)
ENTRY(tfork, S_THREAD)
ENTRY(enable_io, S_ENABIO)
ENTRY(enable_isr, S_ISR)
ENTRY(_mmap, S_MMAP)
ENTRY(munmap, S_MUNMAP)
ENTRY(_strerror, S_STRERROR)
ENTRY(_notify, S_NOTIFY)
ENTRY(clone, S_CLONE)
ENTRY(page_wire, S_PAGE_WIRE)
ENTRY(page_release, S_PAGE_RELEASE)
ENTRY(enable_dma, S_ENABLE_DMA)
ENTRY(time_get, S_TIME_GET)
ENTRY(time_sleep, S_TIME_SLEEP)
ENTRY(dbg_enter, S_DBG_ENTER)
ENTRY(exec, S_EXEC)
ENTRY(waits, S_WAITS)
ENTRY(perm_ctl, S_PERM_CTL)
ENTRY(set_swapdev, S_SET_SWAPDEV)
ENTRY(run_qio, S_RUN_QIO)
ENTRY(set_cmd, S_SET_CMD)
ENTRY(pageout, S_PAGEOUT)
ENTRY(_getid, S_GETID)
ENTRY(unhash, S_UNHASH)
ENTRY(time_set, S_TIME_SET)
ENTRY(ptrace, S_PTRACE)
ENTRY(msg_portname, S_MSG_PORTNAME)
ENTRY(pstat, S_PSTAT)
ENTRY(sched_op, S_SCHED_OP)

/*
 * notify_handler()
 *	Insert a little assembly in front of C event handling
 *
 * The kernel calls the named routine with no saved context.  Put
 * an assembly front-line handler around the C routine, which saves
 * and restore context.
 */
	.data
c_handler: .space	4
	.text
asm_handler:
	pusha				/* Save state */
	pushf
	lea	0x28(%esp),%eax		/* Point to event string */
	push	%eax			/* Leave as arg to routine */
	movl	c_handler,%eax
	call	%eax
	lea	4(%esp),%esp		/* Drop arg */
	popf				/* Restore state */
	popa
	pop	%esp			/* Skip event string */
	ret				/* Resume at old IP */

	.globl	_notify_handler
_notify_handler:
	movl	4(%esp),%eax		/* Get func pointer */
	movl	%eax,c_handler		/* Save in private space */
	movl	$asm_handler,%eax	/* Vector to assembly handler */
	movl	%eax,4(%esp)
	movl	$(S_NOTIFY_HANDLER),%eax
	int	$0xFF
	jc	syserr
	ret
@


1.30
log
@Merge Dave Hudson's changes for unaligned block I/O support
and fixes for errno emulation.
@
text
@d73 1
@


1.29
log
@Add parens to keep gcc 2.X cpp happy
@
text
@d15 4
a18 4
 * The point of this routine is NOT to get the error string; we
 * merely clear the current error string from user space, so that
 * a subsequent strerror() call will know that it must ask the
 * kernel anew about the value.
d21 3
a23 2
	.globl	___err
	.globl	__errcnt
d25 6
a30 2
syserr:	movb	$0,___err
	incl	__errcnt
@


1.28
log
@Add mmap() wrapper to convert file descriptor to port_t
@
text
@d99 1
a99 1
	movl	$S_NOTIFY_HANDLER,%eax
@


1.27
log
@Add assembly wrapper for C event handling
@
text
@d44 1
a44 1
ENTRY(mmap, S_MMAP)
@


1.26
log
@Add signal handling syscall
@
text
@d68 35
a102 1
ENTRY(notify_handler, S_NOTIFY_HANDLER)
@


1.25
log
@Add pstat()
@
text
@d68 1
@


1.24
log
@Unneeded includsion of assym.h in os build
@
text
@d67 1
@


1.23
log
@Remove trailing blank--confuses old gas
@
text
@a9 1
#include "../../os/make/assym.h"
@


1.22
log
@Add msg_portname()
@
text
@a67 1

@


1.21
log
@Add errno count to aid errno.h emulation
@
text
@d67 1
@


1.20
log
@Keep some flavors of gcc from adding a gratuitous space and
breaking gas.
@
text
@d23 1
d26 1
@


1.19
log
@Add ptrace()
@
text
@d28 1
a28 1
	_##n: movl $(v),%eax ; int $0xFF ; jc syserr; ret
@


1.18
log
@Source reorg
@
text
@d64 1
@


1.17
log
@Add time_set()
@
text
@d10 1
a10 1
#include <make/assym.h>
@


1.16
log
@Fix reference to macro parameter
@
text
@d63 1
@


1.15
log
@unhash() syscall
@
text
@d28 1
a28 1
	_##n: movl $v,%eax ; int $0xFF ; jc syserr; ret
@


1.14
log
@Add getid()
@
text
@d62 1
@


1.13
log
@Add pageout daemon entry
@
text
@d61 1
@


1.12
log
@Add set_cmd
@
text
@d60 1
@


1.11
log
@Add swap and qio interfaces
@
text
@d59 1
@


1.10
log
@Add clearing of local error field on kernel error (carry bit)
@
text
@d57 2
@


1.9
log
@Add perm_ctl()
@
text
@d12 15
d28 1
a28 1
	_##n: movl $v,%eax ; int $0xFF ; ret
@


1.8
log
@New syscall waits()
@
text
@d41 1
@


1.7
log
@Oops, EBX would need to be saved; go back to old way, since it
didn't work for thread forks anyway.
@
text
@d40 1
@


1.6
log
@Add kernel debugger entry, and exec()
@
text
@d13 1
a13 1
	_##n: popl %ebx ; movl $v,%eax ; int $0xFF ; jmp %ebx
@


1.5
log
@Need a wrapper for strerror
@
text
@d38 2
@


1.4
log
@Add time syscalls
@
text
@d30 1
a30 1
ENTRY(strerror, S_STRERROR)
@


1.3
log
@Add physical I/O support
@
text
@d36 2
@


1.2
log
@Get return address off stack before syscall, in support of
tfork().
@
text
@d33 4
@


1.1
log
@Initial revision
@
text
@d4 4
d13 1
a13 1
	_##n: movl $v,%eax ; int $0xFF ; ret
@
