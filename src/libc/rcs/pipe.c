head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.04.23.19.48.27;	author vandys;	state Exp;
branches;
next	;


desc
@pipe() emulation using pipe server
@


1.1
log
@Initial revision
@
text
@/*
 * pipe.c
 *	Set up pipe using pipe manager
 */
#include <fcntl.h>

/*
 * pipe()
 *	Set up pipe, place file descriptor value in array
 */
pipe(int *fds)
{
	int fdw, fdr;
	char *p, buf[80];
	extern char *rstat();

	/*
	 * Create a new node, for reading/writing.  Reading, so we
	 * can do the rstat() and get its inode number.
	 */
	fdw = open("/pipe/#", O_RDWR|O_CREAT);
	if (fdw < 0) {
		return(-1);
	}

	/*
	 * Get its "inode number"
	 */
	p = rstat(__fd_port(fdw), "inode");
	if (p == 0) {
		close(fdw);
		return(-1);
	}

	/*
	 * Open a distinct file descriptor for reading
	 */
	sprintf(buf, "/pipe/%s", p);
	if ((fdr = open(buf, O_READ)) < 0) {
		close(fdw);
		return(-1);
	}

	/*
	 * Fill in pair of fd's
	 */
	fds[0] = fdr;
	fds[1] = fdw;

	return(0);
}
@
