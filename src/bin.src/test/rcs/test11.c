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
date	93.04.23.18.13.08;	author vandys;	state Exp;
branches;
next	;


desc
@Test for isatty() working
@


1.1
log
@Initial revision
@
text
@/*
 * Try out scanf
 */
extern char *rstat();

main()
{
	int x;
	char *p;

	printf("isatty stdin -> %d\n", isatty(0));
	p = rstat(__fd_port(0), "type");
	printf("type stdin -> '%s'\n", p ? p : "(null)");
	printf("Enter number:\n");
	scanf("%d", &x);
	printf("Got: %d\n", x);
	return(0);
}
@
