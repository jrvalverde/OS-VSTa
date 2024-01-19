head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.05.30.21.20.52;	author vandys;	state Exp;
branches;
next	;


desc
@Dummy <sys/stat.h>
@


1.1
log
@Initial revision
@
text
@/*
 * <sys/stat.h>
 *	Dummy file to satisfy programs which include it here
 *
 * Mandatory POSIX gripe: should've moved this cr*p out of <sys/*>.
 */
#include <stat.h>
@
