head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.11.20.00.59.20;	author vandys;	state Exp;
branches;
next	;


desc
@Make directories
@


1.1
log
@Initial revision
@
text
@/*
 * mkdir.c
 *	Create directories
 */
main(int argc, char **argv)
{
	int x, errs = 0;
	
	for (x = 1; x < argc; ++x) {
		if (mkdir(argv[x]) < 0) {
			perror(argv[x]);
			errs++;
		}
	}
	return(errs);
}
@
