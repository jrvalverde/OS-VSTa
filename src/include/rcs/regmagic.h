head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.08.02.23.59.43;	author vandys;	state Exp;
branches;
next	;


desc
@Internal defs, regexp library
@


1.1
log
@Initial revision
@
text
@/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define	MAGIC	0234
@
