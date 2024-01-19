head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.4
date	94.05.30.21.29.53;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.29.22.54.48;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.24.19.09.52;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.00.33;	author vandys;	state Exp;
branches;
next	;


desc
@printf() and friends.
@


1.4
log
@varargs stuff
@
text
@/*
 * printf.c
 *	printf/sprintf implementations
 *
 * Use the underlying __doprnt() routine for their dirty work
 */
#include <stdio.h>
#include <std.h>
#include <sys/param.h>

extern void __doprnt();

/*
 * __fprintf()
 *	Formatted output to a FILE, with args in array form
 */
static
__fprintf(FILE *fp, const char *fmt, va_list argptr)
{
	char buf[BUFSIZ], *p, c;

	__doprnt(buf, fmt, (int *)argptr);
	p = buf;
	while (c = *p++) {
		putc(c, fp);
	}
	return(0);
}

/*
 * fprintf()
 *	Formatted output to a FILE
 */
fprintf(FILE *fp, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);	
	return(__fprintf(fp, fmt, ap));
}

/*
 * printf()
 *	Output to stdout
 *
 * This one is only used when you run printf() without using stdio.h.
 */
printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	return(__fprintf(stdout, fmt, ap));
}

/*
 * sprintf()
 *	Formatted output to a buffer
 */
sprintf(char *buf, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	__doprnt(buf, fmt, (int *)ap);
	return(0);
}

/*
 * vfprintf()
 *	Formatted output to a FILE
 */
vfprintf(FILE *fp, const char *fmt, va_list ap)
{
	return(__fprintf(fp, fmt, ap));
}

/*
 * vprintf()
 *	Output to stdout
 *
 * This one is only used when you run printf() without using stdio.h.
 */
vprintf(const char *fmt, va_list ap)
{
	return(__fprintf(stdout, fmt, ap));
}

/*
 * vsprintf()
 *	Formatted output to a buffer
 */
vsprintf(char *buf, const char *fmt, va_list ap)
{
	__doprnt(buf, fmt, (int *)ap);
	return(0);
}

/*
 * perror()
 *	Print out error to stderr
 */
void
perror(const char *msg)
{
	char *p;

	p = strerror();
	fprintf(stderr, "%s: %s\n", (int)msg, p);
}
@


1.3
log
@Add const modifier for perror(), add flag to inhibit
collision between external prototype and internal
implementation.
@
text
@a6 1
#define __PRINTF_INTERNAL
d18 1
a18 1
__fprintf(FILE *fp, char *fmt, int *argptr)
d22 1
a22 1
	__doprnt(buf, fmt, argptr);
d34 1
a34 1
fprintf(FILE *fp, char *fmt, int arg0, ...)
d36 4
a39 1
	return(__fprintf(fp, fmt, &arg0));
d48 1
a48 1
printf(char *fmt, int arg0, ...)
d50 4
a53 1
	return(__fprintf(stdout, fmt, &arg0));
d60 34
a93 1
sprintf(char *buf, char *fmt, int arg0, ...)
d95 1
a95 1
	__doprnt(buf, fmt, &arg0);
@


1.2
log
@Use prototypes, fix calling error strerror()
@
text
@d7 1
d66 1
a66 1
perror(char *msg)
@


1.1
log
@Initial revision
@
text
@d8 1
d67 1
a67 1
	char errbuf[ERRLEN];
d69 2
a70 5
	(void)strerror(errbuf);
	if (errbuf[0] == '\0') {
		strcpy(errbuf, "no error");
	}
	fprintf(stderr, "%s: %s\n", (int)msg, errbuf);
@
