head	1.13;
access;
symbols
	V1_3_1:1.11
	V1_3:1.11
	V1_2:1.7
	V1_1:1.7
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.13
date	94.10.04.20.27.34;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.09.27.21.32.38;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.04.06.00.34.58;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.03.28.23.19.50;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.03.23.21.52.46;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.02.27.02.30.22;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.06.30.19.54.36;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.04.12.20.55.03;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.09.17.11.51;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.24.21.21.38;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.24.19.10.25;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.15.36.02;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.02.05;	author vandys;	state Exp;
branches;
next	;


desc
@C wrappers for system calls.  Most common use is to count up
null-terminated user strings and provide a count to the kernel.
@


1.13
log
@Merge Dave Hudson's changes for unaligned block I/O support
and fixes for errno emulation.
@
text
@/*
 * syscall.c
 *	C code to do a little massaging before firing the "true" syscall
 */
#include <sys/types.h>
#include <sys/fs.h>

extern pid_t _getid(int);

char __err[ERRLEN] = "";	/* Latest error string */
int _errno = 0;			/* Simulation for POSIX errno */
int _old_errno = 0;		/* Last used value of the POSIX errno */
int _err_sync = 0;		/* Used to sync errors in kernel and libc */

/*
 * msg_err()
 *	Count string length, then invoke system call
 *
 * It's a lot harder to do in kernel mode, so count length in the system
 * call layer.
 */
msg_err(long port, char *errmsg)
{
	extern int _msg_err();

	return(_msg_err(port, errmsg, strlen(errmsg)));
}

/*
 * notify()
 *	Similarly for notify
 */
notify(long pid, long tid, char *event)
{
	extern int _notify();

	return(_notify(pid, tid, event, strlen(event)));
}

/*
 * exit()
 *	Flush I/O buffers, then _exit()
 */
void volatile
exit(int val)
{
	extern void volatile _exit(int);
	extern void __allclose();

	__allclose();
	for (;;) {
		_exit(val & 0xFF);
	}
}

/*
 * abort()
 *	Croak ourselves
 */
void volatile
abort(void)
{
	extern int notify();

	/*
	 * First send a generic event.  Follow with non-blockable death.
	 */
	notify(0L, 0L, "abort");
	for (;;) {
		notify(0L, 0L, EKILL);
	}
}

/*
 * strerror()
 *	Get error string
 *
 * The error string comes from one of two places; if __err[] is
 * empty, we query the kernel.  Otherwise we use its current value.
 * This allows us to set a system error from the C library.  Since
 * we emulate much of what is traditionally kernel functionality in
 * the C library, it is necessary for us to be able to set "system"
 * errors.
 */
char *
strerror()
{
	/*
	 * This is a bit of a hack - we would include <errno.h>, but there
	 * are some #define'd constants with the same names, but different
	 * values as those in <sys/fs.h>
	 */
	extern int *__ptr_errno(void);

	/*
	 * We first check whether or not there's been an "errno" change
	 * since the last error string check/modification.  If there has,
	 * the new errno represents the latest error and should be used
	 */
	if (_errno != _old_errno) {
		int x = *__ptr_errno();
	} else if (_err_sync) {
		if (_strerror(__err) < 0) {
			strcpy(__err, "fault");
			_err_sync = 0;
		}
	}

	return(__err);
}
 
/*
 * __seterr()
 *	Set error string to given value
 *
 * Used by internals of C library to set our error state.  We then inform
 * the kernel to keep matters straight.
 */
__seterr(char *p)
{
	/*
	 * We need to make sure that the errno emulation won't become confused
	 * by what we're doing.  We also need to override any kernel messages
	 */
	_old_errno = _errno;
	_err_sync = 0;

	if (!p) {
		__err[0] = '\0';
	} else if (strlen(p) >= ERRLEN) {
		abort();
	} else {
		strcpy(__err, p);
	}

	return(-1);
}

/*
 * getpid()/gettid()/getppid()
 *	Get PID, TID, PPID
 */
pid_t
getpid(void)
{
	return(_getid(0));
}
pid_t
gettid(void)
{
	return(_getid(1));
}
pid_t
getppid(void)
{
	return(_getid(2));
}
void *
mmap(void *vaddr, ulong len, int prot, int flags, int fd, ulong offset)
{
	extern void *_mmap();

	return(_mmap(vaddr, len, prot, flags,
		fd ? __fd_port(fd) : 0, offset));
}
@


1.12
log
@Add mmap() wrapper to convert file descriptor to port_t
@
text
@d10 4
a13 3
char __err[ERRLEN];	/* Latest error string */
uint _errcnt;		/* Bumped on each error (errno.h emulation) */
int _errno;		/* Simulation for POSIX errno */
d88 6
d95 11
a105 4
	if (__err[0] == '\0') {
		if ((_strerror(__err) < 0) || !__err[0]) {
			_errcnt += 1;
			strcpy(__err, "unknown error");
d108 1
d111 1
a111 1

d116 2
a117 2
 * Used by internals of C library to set our error without involving
 * the kernel.
d121 10
a130 1
	if (strlen(p) >= ERRLEN) {
d132 2
d135 1
a135 2
	strcpy(__err, p);
	_errcnt += 1;
@


1.11
log
@Remove "..."; breaks GCC 1.X
@
text
@d133 8
@


1.10
log
@Errno emulation improvements
@
text
@d85 1
a85 1
strerror(...)
@


1.9
log
@Go to vararg so strerror() will fool POSIX apps
@
text
@d12 1
d90 1
d110 1
@


1.8
log
@Add storage for _errcnt
@
text
@d84 1
a84 1
strerror(void)
@


1.7
log
@GCC warning cleanup
@
text
@d11 1
@


1.6
log
@Use getid() to synthesize various ID functions
@
text
@d41 1
a41 1
void
d44 1
a44 1
	extern int _exit();
d48 3
a50 1
	_exit(val & 0xFF);
d57 1
a57 1
void
d66 3
a68 2
	notify(0L, 0L, EKILL);
	/*NOTREACHED*/
@


1.5
log
@Truncate exit value to 8 bits
@
text
@d8 2
d105 20
@


1.4
log
@Get rid of stray debug printf's
@
text
@d46 1
a46 1
	_exit(val);
@


1.3
log
@Fix up strerror() function.  Add interface to set error from
C library.
@
text
@a81 1
		printf("Get from kern\n");
a85 1
	printf("Err is %s\n", __err);
@


1.2
log
@strerror() wrapper because we need a static char buffer
@
text
@d8 2
d14 1
a14 1
 * It's a lot harder to do in kernel mode, so do it in the system
d68 8
a75 1
 *	Get string error from kernel
d80 10
a89 1
	static char err[ERRLEN];
d91 11
a101 3
	err[0] = '\0';
	if ((_strerror(err) < 0) || !err[0]) {
		strcpy(err, "unknown error");
d103 2
a104 1
	return(err);
@


1.1
log
@Initial revision
@
text
@d63 16
@
