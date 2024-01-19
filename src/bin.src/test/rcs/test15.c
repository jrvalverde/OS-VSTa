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
date	93.09.30.20.35.58;	author vandys;	state Exp;
branches;
next	;


desc
@Test15
@


1.1
log
@Initial revision
@
text
@#include <stdio.h>

main()
{
	FILE *fp;
	char buf[80];
	int x;

	fp = fopen("test.c", "r");
	x = fread(buf, 10, 1, fp);
	printf("Read of 10 gets: %d\n", x);
	write(1, buf, 10);
	printf("\nUnget '*'\n");
	ungetc('*', fp);
	x = fread(buf, 10, 1, fp);
	printf("Read of 10 gets: %d\n", x);
	write(1, buf, 10);
	fclose(fp);
	return(0);
}
@
