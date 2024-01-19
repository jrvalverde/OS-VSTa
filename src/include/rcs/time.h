head	1.7;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.7
date	94.07.10.19.18.39;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.07.10.19.17.30;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.03.23.21.53.10;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.02.02.19.42.03;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.02.01.23.24.01;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.11.19.18.11;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.09.23.29.53;	author vandys;	state Exp;
branches;
next	;


desc
@Def interfaces for standards-driven time
@


1.7
log
@Add utime/utimes
@
text
@#ifndef _TIME_H
#define _TIME_H
/*
 * time.h
 *	Interface for timing information
 *
 * Some of this based on technology:
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 */
typedef long time_t;

struct tm {
	int	tm_sec;		/* seconds after the minute [0-60] */
	int	tm_min;		/* minutes after the hour [0-59] */
	int	tm_hour;	/* hours since midnight [0-23] */
	int	tm_mday;	/* day of the month [1-31] */
	int	tm_mon;		/* months since January [0-11] */
	int	tm_year;	/* years since 1900 */
	int	tm_wday;	/* days since Sunday [0-6] */
	int	tm_yday;	/* days since January 1 [0-365] */
	int	tm_isdst;	/* Daylight Savings Time flag */
	long	tm_gmtoff;	/* offset from CUT in seconds */
	char	*tm_zone;	/* timezone abbreviation */
};

struct timeval {
	time_t	tv_sec;		/* Seconds and uSeconds */
	time_t	tv_usec;
};

/*
 * Some prototypes
 */
extern char *ctime(time_t *);
extern struct tm *gmtime(time_t *), *localtime(time_t *);
extern int __usleep(int);
extern int __msleep(int);
extern time_t time(time_t *);
extern int utimes(const char *, struct timeval *);

#endif /* _TIME_H */
@


1.6
log
@Add timeval definition
@
text
@d40 1
@


1.5
log
@Add time() proto
@
text
@d27 5
@


1.4
log
@use standard C type to avoid tedium of including sys types
@
text
@d34 1
@


1.3
log
@Add protos
@
text
@d32 2
a33 2
extern int __usleep(uint);
extern int __msleep(uint);
@


1.2
log
@Add "struct tm" and supporting prototypes
@
text
@d32 2
@


1.1
log
@Initial revision
@
text
@d6 4
d13 19
a31 1
extern char *ctime(long *);
@
