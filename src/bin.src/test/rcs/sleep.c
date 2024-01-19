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
date	93.10.17.19.27.15;	author vandys;	state Exp;
branches;
next	;


desc
@Sleep command
@


1.1
log
@Initial revision
@
text
@/*
 * sleep.c
 *	Fall asleep for a bit
 */
#include <stdio.h>

main(int argc, char **argv)
{
	int secs;

	if (argc != 2) {
		fprintf(stderr, "Usage is: %s <seconds>\n", argv[0]);
		exit(1);
	}
	if (sscanf(argv[1], "%d", &secs) != 1) {
		fprintf(stderr, "Invalid number: %s\n", argv[1]);
		exit(1);
	}
	(void)sleep(secs);
	exit(0);
}
@
