head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.10.05.20.25.48;	author vandys;	state Exp;
branches;
next	;


desc
@Initial RCS checkin of Mike Larson's CDFS
@


1.1
log
@Initial revision
@
text
@/*
 * cdfserror.c - print/log CDFS error messages.
 */
#include <stdio.h>
#include <stdarg.h>
#include <std.h>
#include <syslog.h>
#include "cdfs.h"

static	char msgbuf[128];

void	cdfs_error(uint flags, char *myname, char *fmt, ...)
{
	va_list	ap;
	int	a1, a2, a3, a4;

	sprintf(msgbuf, "%s: ", myname);
	va_start(ap, fmt);
	a1 = va_arg(ap, int); a2 = va_arg(ap, int);
	a3 = va_arg(ap, int); a4 = va_arg(ap, int);
	sprintf(&msgbuf[strlen(msgbuf)], fmt, a1, a2, a3, a4);
	strcat(msgbuf, "\n");
	syslog(LOG_ERR, msgbuf);
	if(flags & CDFS_PRINT_SYSERR)
		syslog(LOG_ERR, "%s\n", strerror());
}

void	cdfs_debug(uint when, char *myname, char *fmt, ...)
{
	va_list	ap;
	int	a1;

	if(when & cdfs_debug_flags) {
		sprintf(msgbuf, "%s: ", myname);
		if(fmt != NULL) {
			va_start(ap, fmt);
			a1 = va_arg(ap, int);
			sprintf(&msgbuf[strlen(msgbuf)], fmt, a1);
			va_end(ap);
		}
		strcat(msgbuf, "\n");
		syslog(LOG_DEBUG, msgbuf);
	}
}
@
