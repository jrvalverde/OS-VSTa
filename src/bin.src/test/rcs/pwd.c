head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.1
date	94.03.07.17.31.22;	author vandys;	state Exp;
branches;
next	;


desc
@Print Working Directory
@


1.1
log
@Initial revision
@
text
@/*
 * pwd.c
 *	Print current working directory
 */
#include <std.h>

main(void)
{
	char buf[1024], *p;

	p = getcwd(buf, sizeof(buf));
	if (p) {
		printf("%s\n", p);
		return(0);
	} else {
		printf("Can't get CWD\n");
		return(1);
	}
}
@
