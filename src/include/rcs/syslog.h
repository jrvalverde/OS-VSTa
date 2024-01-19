head	1.3;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.06.21.20.56.11;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.06.03.04.46.24;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.02.28.19.43.27;	author vandys;	state Exp;
branches;
next	;


desc
@Syslog definitions
@


1.3
log
@Add protos and logging options
@
text
@#ifndef _SYSLOG_H
#define _SYSLOG_H
/*
 * syslog.h
 *	Definitions for system logging facility
 *
 * Be warned; VSTa ignores most of this
 */

/*
 * Main entry for reporting stuff
 */
extern void syslog(int, const char *, ...),
	openlog(char *, int, int);

/*
 * Level values
 */
#define LOG_EMERG (1)		/* Panic */
#define LOG_ALERT (2)		/* Watch out */
#define LOG_CRIT (3)		/* Critical errors */
#define LOG_ERR (4)		/* Oops, but oh well */
#define LOG_WARNING (5)		/* You *were* warned */
#define LOG_NOTICE (6)		/* FYI */
#define LOG_INFO (7)		/* FYI too */
#define LOG_DEBUG (8)		/* Hacker's friend */

/*
 * Logopt bits
 */
#define LOG_PID (1)
#define LOG_CONS (2)
#define LOG_NDELAY (4)
#define LOG_NOWAIT LOG_NDELAY

/*
 * Facility
 */
#define LOG_KERN (1)
#define LOG_USER (2)
#define LOG_MAIL (3)
#define LOG_DAEMON (4)
#define LOG_AUTH (5)
#define LOG_LPR (6)
#define LOG_NEWS (7)
#define LOG_LOCAL (8)

#endif /* _SYSLOG_H */
@


1.2
log
@Add const qualifier
@
text
@d13 2
a14 1
extern void syslog(int, const char *, ...);
d27 20
@


1.1
log
@Initial revision
@
text
@d13 1
a13 1
extern void syslog(int, char *, ...);
@
