head	1.6;
access;
symbols
	V1_3_1:1.3
	V1_3:1.2;
locks; strict;
comment	@ * @;


1.6
date	94.06.21.20.55.53;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.06.03.04.46.07;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.05.30.21.29.53;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.20.21.05.10;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.03.01.17.24.07;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.02.28.20.02.13;	author vandys;	state Exp;
branches;
next	;


desc
@Syslog facility (baby step)
@


1.6
log
@Add openlog() and support for logging options
@
text
@/*
 * syslog.c
 *	A (very) poor man's syslog handler
 */
#include <sys/fs.h>
#include <syslog.h>
#include <stdio.h>
#include <std.h>

static char *id;		/* Name os process posting message */
static int logopt, logfacil;	/* Logging options & facility */

/*
 * levelmsg()
 *	Convert numeric level to string
 */
static char *
levelmsg(int level)
{
	switch (level) {
	case LOG_EMERG: return("emergency");
	case LOG_ALERT: return("alert");
	case LOG_CRIT: return("critical");
	case LOG_ERR: return("error");
	case LOG_WARNING: return("warning");
	case LOG_NOTICE: return("notice");
	case LOG_INFO: return("info");
	case LOG_DEBUG: return("debug");
	default: return("unknown");
	}
}

/*
 * openlog()
 *	Initialize for syslog(), record identity
 */
void
openlog(char *ident, int opt, int facil)
{
	id = strdup(ident);
	logopt = opt;
	logfacil = facil;
}

/*
 * pidstr()
 *	Return string representation of PID
 *
 * Returns empty string of LOG_PID not specified
 */
static char *
pidstr(void)
{
	static ulong pid;
	static char str[16];

	if ((logopt & LOG_PID) == 0) {
		return("");
	}
	if (pid == 0) {
		pid = getpid();
		sprintf(str, "(pid %ld) ", pid);
	}
	return(str);
}

/*
 * syslog()
 *	Report error conditions
 *
 * We just dump them to the console
 */
void
syslog(int level, const char *msg, ...)
{
	port_t p;
	int fd;
	char buf[256];
	va_list ap;

	p = path_open("CONS:0", ACC_WRITE);
	if (p < 0) {
		return;
	}
	fd = __fd_alloc(p);
	va_start(ap, msg);
	sprintf(buf, "syslog: %s %s%s: ",
		id ? id : "", pidstr(), levelmsg(level));
	vsprintf(buf + strlen(buf), msg, ap);
	if (buf[strlen(buf)-1] != '\n') {
		strcat(buf, "\n");
	}
	write(fd, buf, strlen(buf));
	close(fd);
}
@


1.5
log
@Fix varargs stuff (yuck)
@
text
@d8 1
d10 3
d34 34
d87 2
a88 1
	sprintf(buf, "syslog: %s: ", levelmsg(level));
@


1.4
log
@varargs stuff
@
text
@d41 1
a41 1
	va_args ap;
d50 1
a50 1
	sprintf(buf + strlen(buf), msg, ap);
@


1.3
log
@Add missing newline, correct "warnings" to "warning"
@
text
@d7 1
a9 5
 * Our bane of existence, varargs, and our way to avoid it
 */
#define ARG(start, idx) (*((void **)&(start) + (idx)))

/*
d36 1
a36 1
syslog(int level, char *msg, ...)
d41 1
d48 1
d50 1
a50 3
	sprintf(buf + strlen(buf), msg,
		ARG(msg, 1), ARG(msg, 2), ARG(msg, 3),
		ARG(msg, 4), ARG(msg, 5), ARG(msg, 6));
@


1.2
log
@Tidy up printing, allow more args
@
text
@d25 1
a25 1
	case LOG_WARNING: return("warnings");
d55 3
@


1.1
log
@Initial revision
@
text
@d52 3
a54 1
	sprintf(buf, msg, ARG(msg, 1), ARG(msg, 2), ARG(msg, 3));
@
