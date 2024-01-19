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
date	94.10.13.14.18.19;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.07.10.18.24.44;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.02.01.57.10;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.00.25.28;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.48.12;	author vandys;	state Exp;
branches;
next	;


desc
@File control stuff
@


1.5
log
@Add some const declarations
@
text
@#ifndef _FCNTL_H
#define _FCNTL_H
/*
 * fcntl.h
 *	Definitions for control of files
 */
#include <sys/types.h>

/*
 * Options for an open()
 */
#define O_READ (1)
#define O_RDONLY O_READ
#define O_WRITE (2)
#define O_WRONLY O_WRITE
#define O_RDWR (O_READ|O_WRITE)
#define O_CREAT (4)
#define O_TRUNC O_CREAT
#define O_EXCL (8)
#define O_APPEND (16)
#define O_TEXT (0)
#define O_BINARY (32)
#define O_DIR (64)

/*
 * Max # characters in a path.  Not clear this belongs here.
 */
#define MAXPATH (512)

extern int open(const char *, int, ...), close(int),
	creat(const char *, int),
	read(int fd, void *buf, int nbyte),
	write(int fd, const void *buf, int nbyte),
	close(int), mkdir(const char *),
	unlink(const char *);
extern off_t lseek(int fd, off_t offset, int whence);

#endif /* _FCNTL_H */
@


1.4
log
@Add rmdir(), add some const definitions
@
text
@d31 1
a31 1
	creat(char *, int),
d33 2
a34 2
	write(int fd, void *buf, int nbyte),
	close(int), mkdir(char *),
a38 1

@


1.3
log
@Add creat()
@
text
@d30 1
a30 1
extern int open(char *, int, ...), close(int),
d35 1
a35 1
	unlink(char *);
@


1.2
log
@Add unlink() prototypes
@
text
@d31 1
d39 1
@


1.1
log
@Initial revision
@
text
@d33 2
a34 1
	close(int), mkdir(char *);
@
