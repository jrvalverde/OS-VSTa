head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.4
date	94.10.13.14.18.19;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.03.28.23.09.12;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.08.17.49.10;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.49.12;	author vandys;	state Exp;
branches;
next	;


desc
@Yet Another POSIX definitions file
@


1.4
log
@Add some const declarations
@
text
@#ifndef _UNISTD_H
#define _UNISTD_H
/*
 * unistd.h
 *	A standards-defined file
 *
 * I don't have a POSIX manual around here!  I'll just throw in some
 * stuff I'm pretty sure it needs and hope to borrow a manual some
 * time soon.
 */

/*
 * Third argument to lseek (and ftell)
 */
#define SEEK_SET (0)
#define SEEK_CUR (1)
#define SEEK_END (2)

/*
 * Second argument to access()
 */
#define R_OK (4)
#define W_OK (2)
#define X_OK (1)
#define F_OK (0)

/*
 * Standard file number definitions
 */
#define STDIN_FILENO (0)
#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

/*
 * Function protos
 */
extern char *getlogin(void);

#endif /* _UNISTD_H */
@


1.3
log
@Add some more POSIX stuff
@
text
@d34 5
@


1.2
log
@Add bits for access()
@
text
@d27 7
@


1.1
log
@Initial revision
@
text
@d19 8
@
