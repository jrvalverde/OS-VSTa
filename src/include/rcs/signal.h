head	1.5;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	94.05.30.21.39.57;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.05.30.21.31.20;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.03.30.17.57.29;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.20.56.38;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.09.17.42.23;	author vandys;	state Exp;
branches;
next	;


desc
@Signal stuff
@


1.5
log
@Leave out defs for job control signals
@
text
@#ifndef _SIGNAL_H
#define _SIGNAL_H
/*
 * signal.h
 *	A hokey little mapping from VSTa events into numbered signals
 */
#include <sys/types.h>

/*
 * Default and ignore signal "handlers"
 */
typedef voidfun sig_t;
#define SIG_DFL ((voidfun)(-1))
#define SIG_IGN ((voidfun)(-2))

/*
 * Some of these are not yet used in VSTa, but are included for completeness
 */
#define SIGHUP 1	/* Hangup */
#define SIGINT 2	/* Keyboard interrupt */
#define SIGQUIT 3	/* Keyboard abort */
#define SIGILL 4	/* Illegal instruction */
#define SIGTRAP 5
#define SIGABRT 6
#define SIGIOT SIGABRT
#define SIGUNUSED 7
#define SIGFPE 8	/* Floating point exception */
#define SIGKILL 9	/* Unmaskable kill */
#define SIGUSR1 10
#define SIGSEGV 11	/* Segmentation violation */
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15	/* Software termination */
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCLD SIGCHLD
/* #define SIGCONT 18	These make us appear to support job control */
/* #define SIGSTOP 19 */
/* #define SIGTSTP 20 */
/* #define SIGTTIN 21 */
/* #define SIGTTOU 22 */
#define SIGIO 23
#define SIGPOLL SIGIO
#define SIGURG SIGIO
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGLOST 29
#define SIGPWR 30
#define SIGBUS 31

#define _NSIG 32	/* Max # emulated signals */

extern voidfun signal(int, voidfun);
extern int kill(pid_t, int);

#endif /* _SIGNAL_H */
@


1.4
log
@Add more emulated signal values
@
text
@d37 6
a42 5
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
@


1.3
log
@Add sig_t
@
text
@d16 3
d23 4
d29 1
d31 3
d35 18
d54 1
a54 1
#define _NSIG 16	/* Max # emulated signals */
@


1.2
log
@Tidy up voidfun usage, add signal()/kill() prototypes
@
text
@d12 1
@


1.1
log
@Initial revision
@
text
@d7 1
a11 1
typedef void (*__voidfun)();
d26 2
a27 1
extern __voidfun signal(int, __voidfun);
@
