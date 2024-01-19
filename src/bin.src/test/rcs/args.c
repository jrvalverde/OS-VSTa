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
date	93.02.25.21.22.54;	author vandys;	state Exp;
branches;
next	;


desc
@Simple program to look out our expanded arguments
@


1.1
log
@Initial revision
@
text
@main(argc, argv)
	int argc;
	char **argv;
{
	int x;

	printf("argc = %d\n", argc);
	for (x = 0; x < argc; ++x) {
		printf(" argv[%d] = '%s'\n", x, argv[x]);
	}
	exit(0);
}
@
