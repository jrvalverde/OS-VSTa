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
date	93.01.29.15.56.27;	author vandys;	state Exp;
branches;
next	;


desc
@Fluff for handling filenames as provided by a user program
@


1.1
log
@Initial revision
@
text
@/*
 * files.c
 *	Support routines, mostly for sanity checking
 */

/*
 * valid_fname()
 *	Given a message buffer, catch some common crockery
 *
 * Not a completely honest routine name, since we ensure the
 * the presence of a terminating null, too.
 */
valid_fname(char *buf, int bufsize)
{
	if (bufsize > 128) {
		return(0);
	}
	if (buf[bufsize-1] != '\0') {
		return(0);
	}
	return(1);
}
@
