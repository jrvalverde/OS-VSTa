head	1.7;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6;
locks; strict;
comment	@ * @;


1.7
date	94.10.04.20.27.34;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.28.23.50.34;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.03.28.23.19.50;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.03.23.21.52.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.03.15.22.05.43;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.02.27.02.30.35;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.02.27.02.21.50;	author vandys;	state Exp;
branches;
next	;


desc
@Errno mapping code
@


1.7
log
@Merge Dave Hudson's changes for unaligned block I/O support
and fixes for errno emulation.
@
text
@/*
 * errno.c
 *	Compatibility wrapper mapping VSTa errors into old-style values
 *
 * We dance a fine line here, in which we attempt to detect when true,
 * new errors have come up from the kernel, while preserving errno
 * when such has not occurred.
 */
#include <sys/types.h>
#include <errno.h>
#include <std.h>

static struct {
	int errnum;
	char *errstr;
} errmap[] = {
	{ 0, "" },
	{ EPERM, "perm" },
	{ ENOENT, "no file" },
	{ ESRCH, "no entry" },
	{ EINTR, "intr" },
	{ EIO, "io err" },
	{ ENXIO, "no io" },
	{ E2BIG, "too big" },
	{ ENOEXEC, "exec fmt" },
	{ EBADF, "bad file" },
	{ ECHILD, "no child" },
	{ EAGAIN, "again" },
	{ ENOMEM, "no mem" },
	{ EACCES, "access" },
	{ EFAULT, "fault" },
	{ ENOTBLK, "not blk dev" },
	{ EBUSY, "busy" },
	{ EEXIST, "exists" },
	{ EXDEV, "cross dev link" },
	{ ENODEV, "not dev" },
	{ ENOTDIR, "not dir" },
	{ EISDIR, "is dir" },
	{ EINVAL, "invalid" },
	{ ENFILE, "file tab ovfl" },
	{ EMFILE, "too many files" },
	{ ENOTTY, "not tty" },
	{ ETXTBSY, "txt file busy" },
	{ EFBIG, "file too large" },
	{ ENOSPC, "no space" },
	{ ESPIPE, "ill seek" },
	{ EROFS, "RO fs" },
	{ EMLINK, "too many links" },
	{ EPIPE, "broken pipe" },
	{ EDOM, "math domain" },
	{ ERANGE, "math range" },
	{ EMATH, "math" },
	{ EILL, "ill instr" },
	{ EKILL, "kill" },
	{ EBALIGN, "blk align" },
	{ ESYMLINK, "symlink" },
	{ ELOOP, "symlink loop" },
	{ 0, 0 }
};

/*
 * map_errstr()
 *	Given VSTa error string, turn into POSIX errno-type value
 */
static int
map_errstr(char *err)
{
	int x;
	char *p;

	/*
	 * Scan for string match
	 */
	for (x = 0; p = errmap[x].errstr; ++x) {
		if (!strcmp(err, p)) {
			return(errmap[x].errnum);
		}
	}

	/*
	 * Didn't find, just pick something
	 */
	return(EINVAL);
}

/*
 * map_errno()
 *	Given POSIX errno-type value, return a pointer to a VSTa error string
 */
static char *
map_errno(int err)
{
	static char errdef[] = "unknown error";
	int x;
	char *e;

	/*
	 * Scan for string match
	 */
	for (x = 0; e = errmap[x].errstr; ++x) {
		if (errmap[x].errnum == err) {
			return(e);
		}
	}

	/*
	 * Didn't find, just pick something
	 */
	return(errdef);
}

/*
 * __ptr_errno()
 *	Report back the address of errno
 *
 * In order to keep the POSIX errno in step with the VSTa system error
 * message code we need to do some pretty nasty hacks to keep track of what
 * the "errno" value was when we last entered the routine.  If it's changed,
 * either because a new errno has been set, or because a new kernel error has
 * been set we do what we can to put things back in sync
 */
int *
__ptr_errno(void)
{
	extern int _old_errno;
	extern int _errno;
	char *p;

	/*
	 * First we want to know if someone has modified errno.  If they
	 * have, we want to put things back in sync
	 */
	if (_old_errno != _errno) {
		__seterr(map_errno(_errno));
		return(&_errno);
	}

	/*
	 * Get current error.
	 */
	p = strerror();

	/*
	 * Map from string to a value.
	 */
	_old_errno = _errno = map_errstr(p);

	return(&_errno);
}
@


1.6
log
@Improved errno emulation
@
text
@d17 1
d55 4
a58 1
	{0, 0}
d95 1
a95 1
	int e;
d100 3
a102 3
	for (x = 0; e = errmap[x].errnum; ++x) {
		if (e == err) {
			return(errmap[x].errstr);
d119 2
a120 2
 * either because a new errno has been set, or because a new __err has been
 * set we do what we can to put things back in sync
d125 1
a125 2
	static uint old_errcnt;
	static int old_errno;
a126 1
	extern uint _errcnt;
d133 1
a133 1
	if (old_errno != _errno) {
a134 1
		old_errno = _errno;
d139 1
a139 2
	 * Has the system error string been updated?  If not, then we've
	 * already done everything before and simply return the old answer
a140 8
	if (old_errcnt == _errcnt) {
		return(&_errno);
	}
	old_errcnt = _errcnt;

	/*
	 * Get current error.  Don't touch anything if there is none.
	 */
a141 3
	if (!p) {
		return(&_errno);
	}
d146 1
a146 1
	_errno = map_errstr(p);
@


1.5
log
@Errno emulation improvements
@
text
@d18 2
a19 1
	{ ENOENT, "no entry" },
d24 1
d26 1
d29 1
d31 1
d34 2
d37 1
d39 5
d45 1
d47 8
a54 3
	{ EXDEV, "cross dev"},
	{ EISDIR, "is dir"},
	{ 0, 0 }
@


1.4
log
@Add EISDIR
@
text
@d64 26
d92 6
d103 2
a104 1
	static int my_errno;
d109 12
a120 2
	 * If no new errors, stick with what we have.  Otherwise record
	 * latest errno count, and continue.
d123 1
a123 1
		return(&my_errno);
d132 1
a132 1
		return(&my_errno);
d138 3
a140 2
	my_errno = map_errstr(p);
	return(&my_errno);
@


1.3
log
@Add EXDEV error
@
text
@d34 1
@


1.2
log
@Get it compiling
@
text
@d33 1
@


1.1
log
@Initial revision
@
text
@d11 1
d65 2
a66 1
void __ptr_errno(void)
d71 1
@
