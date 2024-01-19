head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.10.01.04.25.57;	author vandys;	state Exp;
branches;
next	;


desc
@Code to print current time
@


1.1
log
@Initial revision
@
text
@/*
 * time.c
 *	Common routines for setime and date
 */
#include <time.h>
#include <stdio.h>

/*
 * prtime()
 *	Read system clock, show time
 */
void
prtime(void)
{
	time_t t;

	(void)time(&t);
	(void)printf("%s", ctime(&t));
}
@
