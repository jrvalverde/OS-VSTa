head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.5
date	94.10.13.16.44.41;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.02.22.40.32;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.20.55.47;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.09.17.12.28;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.11.19.15.59;	author vandys;	state Exp;
branches;
next	;


desc
@Glue to map signal() onto VSTa events
@


1.5
log
@Fix shadowing of macro parm and function parm
@
text
@/*
 * signal.c
 *	Emulation of POSIX-style signals from VSTa events
 *
 * Amusing to map, since VSTa events are strings.  More thought on
 * what to do with the events which have no POSIX signal number.
 */
#include <signal.h>
#include <sys/fs.h>

/*
 * A slot for each signal
 */
static voidfun sigs[_NSIG];

/*
 * __strtosig()
 *	Convert string into signal number
 */
__strtosig(char *e)
{
#define MAP(s, evname) if (!strcmp(evname, e)) {return(s);};
	MAP(SIGINT, EINTR);
	MAP(SIGFPE, EMATH);
	MAP(SIGKILL, EKILL);
	MAP(SIGSEGV, EFAULT);
#undef MAP
	return(SIGINT);	/* Default */
}

/*
 * sigtostr()
 *	Convert number back to string
 */
static char *
sigtostr(int s)
{
	switch (s) {
	case SIGHUP: return("hup");
	case SIGINT: return(EINTR);
	case SIGQUIT: return("quit");
	case SIGILL: return("instr");
	case SIGFPE: return(EMATH);
	case SIGKILL: return(EKILL);
	case SIGSEGV: return(EFAULT);
	case SIGTERM: return("term");
	default:
		return("badsig");
	}
}

/*
 * signal()
 *	Arrange signal handling
 */
voidfun
signal(int s, voidfun v)
{
	static int init = 0;
	voidfun ov;

	if (!init) {
		/* XXX wire handler */
	}
	if (s >= _NSIG) {
		return((voidfun)-1);
	}
	ov = sigs[s];
	sigs[s] = v;
	return(ov);
}

/*
 * kill()
 *	Send an event
 */
kill(pid_t pid, int sig)
{
	return(notify(pid, 0, sigtostr(sig)));
}
@


1.4
log
@-1 means process group, 0 means all threads in the process
@
text
@d22 1
a22 1
#define MAP(s, e) if (!strcmp(#s, e)) {return(s);};
@


1.3
log
@Make signal mapping globally available, add kill()
@
text
@d79 1
a79 1
	return(notify(pid, -1, sigtostr(sig)));
@


1.2
log
@export strtosig()
@
text
@d14 1
a14 1
static __voidfun sigs[_NSIG];
d56 2
a57 2
__voidfun
signal(int s, __voidfun v)
d60 1
a60 1
	__voidfun ov;
d66 1
a66 1
		return((__voidfun)-1);
d71 9
@


1.1
log
@Initial revision
@
text
@d17 1
a17 1
 * strtosig()
d20 1
a20 2
static
strtosig(char *e)
d28 1
a28 1
	return(-1);
@
