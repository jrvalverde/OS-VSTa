head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.2
date	93.04.08.17.48.54;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.07.21.36.15;	author vandys;	state Exp;
branches;
next	;


desc
@Permission manipulation--mostly ugly glue, since VSTa doesn't use
a UNIX model.
@


1.2
log
@Add access()
@
text
@/*
 * perm.c
 *	Permission/protection mapping
 *
 * This is mostly a study in frustration; the POSIX style of interface
 * disables much of the expressive power of the VSTa protection system.
 */
#include <unistd.h>
#include <fcntl.h>

/*
 * umask()
 *	Set default protection mask
 */
umask(int newmask)
{
	return(0600);
}

/*
 * chmod()
 *	Change mode of file
 */
chmod(char *file, int mode)
{
	return(0);
}

/*
 * access()
 *	Tell if we can access a file
 *
 * Ignores the effective/real dichotomy, since we don't really
 * have it as such.
 */
access(char *file, int mode)
{
	int fd;

	if (mode & W_OK) {
		fd = open(file, O_READ|O_WRITE);
	} else {
		fd = open(file, O_READ);
	}
	if (fd >= 0) {
		close(fd);
		return(0);
	}
	return(-1);
}
@


1.1
log
@Initial revision
@
text
@d8 2
d27 23
@
