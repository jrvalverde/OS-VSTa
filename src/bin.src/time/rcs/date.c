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
@Date command
@


1.1
log
@Initial revision
@
text
@/*
 * date.c
 *	Main routine for date(1) command
 */
extern void prtime(void);

main()
{
	prtime();
}
@
