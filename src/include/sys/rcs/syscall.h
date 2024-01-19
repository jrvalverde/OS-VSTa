head	1.19;
access;
symbols
	V1_3_1:1.16
	V1_3:1.16
	V1_2:1.13
	V1_1:1.12
	V1_0:1.11;
locks; strict;
comment	@ * @;


1.19
date	94.12.21.05.33.00;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	94.08.25.00.57.26;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.07.06.04.44.26;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.03.15.22.06.44;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.03.04.02.02.58;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.02.01.23.23.33;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.12.09.06.19.15;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.10.01.19.06.57;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.04.20.21.24.43;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.04.12.20.54.18;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.03.30.01.08.31;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.03.26.23.38.54;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.03.26.23.29.24;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.20.00.19.34;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.03.23.14.46;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.23.18.17.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.08.15.07.27;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.03.20.12.55;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.17.32;	author vandys;	state Exp;
branches;
next	;


desc
@Definition of syscall numbers
@


1.19
log
@Add scheduler interface for RT/BG/yield
@
text
@#ifndef _SYSCALL_H
#define _SYSCALL_H
/*
 * syscall.h
 *	Values/prototypes for system calls
 *
 * Not all of the system calls have prototypes defined here.  The ones that
 * are missing from here can be found in other (more appropriate) .h files
 */
#define S_MSG_PORT 0
#define S_MSG_CONNECT 1
#define S_MSG_ACCEPT 2
#define S_MSG_SEND 3
#define S_MSG_RECEIVE 4
#define S_MSG_REPLY 5
#define S_MSG_DISCONNECT 6
#define S_MSG_ERR 7
#define S_EXIT 8
#define S_FORK 9
#define S_THREAD 10
#define S_ENABIO 11
#define S_ISR 12
#define S_MMAP 13
#define S_MUNMAP 14
#define S_STRERROR 15
#define S_NOTIFY 16
#define S_CLONE 17
#define S_PAGE_WIRE 18
#define S_PAGE_RELEASE 19
#define S_ENABLE_DMA 20
#define S_TIME_GET 21
#define S_TIME_SLEEP 22
#define S_DBG_ENTER 23
#define S_EXEC 24
#define S_WAITS 25
#define S_PERM_CTL 26
#define S_SET_SWAPDEV 27
#define S_RUN_QIO 28
#define S_SET_CMD 29
#define S_PAGEOUT 30
#define S_GETID 31
#define S_UNHASH 32
#define S_TIME_SET 33
#define S_PTRACE 34
#define S_MSG_PORTNAME 35
#define S_PSTAT 36
#define S_NOTIFY_HANDLER 37
#define S_SCHED_OP 38
#define S_HIGH S_SCHED_OP

/*
 * Some syscall prototypes
 */
#ifndef __ASM__
#include <sys/types.h>

extern int enable_io(int arg_low, int arg_high);
extern int enable_isr(port_t arg_port, int irq);
extern int clone(port_t arg_port);
extern int page_wire(void *arg_va, void **arg_pa);
extern int page_release(uint arg_handle);
extern int enable_dma(int);
extern int time_get(struct time *arg_time);
extern int time_sleep(struct time *arg_time);
extern void dbg_enter(void);
extern int set_swapdev(port_t arg_port);
extern void run_qio(void);
extern int set_cmd(char *arg_cmd);
extern int pageout(void);
extern int unhash(port_t arg_port, long arg_fid);
extern int time_set(struct time *arg_time);
extern int ptrace(pid_t pid, port_name name);
extern int notify_handler(voidfun);

#endif /* __ASM__ */

#endif /* _SYSCALL_H */
@


1.18
log
@Add signal handling syscall
@
text
@d48 2
a49 1
#define S_HIGH S_NOTIFY_HANDLER
@


1.17
log
@Add pstat()
@
text
@d47 2
a48 1
#define S_HIGH S_PSTAT
d72 1
@


1.16
log
@Add msg_portname()
@
text
@d46 2
a47 1
#define S_HIGH S_MSG_PORTNAME
@


1.15
log
@Fix warnings, prepare for -Wall
@
text
@d45 2
a46 1
#define S_HIGH S_PTRACE
@


1.14
log
@Add a bunch of syscall protos
@
text
@d58 1
a58 1
extern int enable_dma(void);
@


1.13
log
@Add ptrace
@
text
@d5 4
a8 1
 *	Values for system calls
d46 25
@


1.12
log
@Add time_set()
@
text
@d41 2
a42 1
#define S_HIGH S_TIME_SET
@


1.11
log
@unhash() syscall
@
text
@d40 2
a41 1
#define S_HIGH S_UNHASH
@


1.10
log
@New interface for manipulating various ID's
@
text
@d39 2
a40 1
#define S_HIGH S_GETID
@


1.9
log
@Add pageout daemon entry
@
text
@d38 2
a39 1
#define S_HIGH S_PAGEOUT
@


1.8
log
@Add set_cmd()
@
text
@d37 2
a38 1
#define S_HIGH S_SET_CMD
@


1.7
log
@Add swap and qio calls
@
text
@d36 2
a37 1
#define S_HIGH S_RUN_QIO
@


1.6
log
@Add perm_ctl() syscall
@
text
@d34 3
a36 1
#define S_HIGH S_PERM_CTL
@


1.5
log
@New syscall waits()
@
text
@d33 2
a34 1
#define S_HIGH S_WAITS
@


1.4
log
@Add exec() support, plus a debug entry for kicking the kernel
debugger.
@
text
@d32 2
a33 1
#define S_HIGH S_EXEC
@


1.3
log
@Add time syscalls
@
text
@d30 3
a32 1
#define S_HIGH S_TIME_SLEEP
@


1.2
log
@Add physical I/O support
@
text
@d28 3
a30 1
#define S_HIGH S_ENABLE_DMA
@


1.1
log
@Initial revision
@
text
@d25 4
a28 1
#define S_HIGH S_CLONE
@
