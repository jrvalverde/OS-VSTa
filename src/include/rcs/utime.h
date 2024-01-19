head	1.2;
access;
symbols;
locks; strict;
comment	@ * @;


1.2
date	94.07.10.19.18.39;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.07.10.18.56.06;	author vandys;	state Exp;
branches;
next	;


desc
@Utime header file
@


1.2
log
@Add utime/utimes
@
text
@#ifndef _UTIME_H
#define _UTIME_H
/*
 * utime.h
 *	Time stamp handling
 *
 * Gee, if there's something we need more of it's time-oriented header
 * files and APIs.  So here's another one.
 */
#include <time.h>

struct utimbuf {
	time_t actime;		/* Access time */
	time_t modtime;		/* Modification time */
};

extern int utime(const char *, struct utimbuf *);

#endif /* _UTIME_H */
@


1.1
log
@Initial revision
@
text
@d16 3
@
