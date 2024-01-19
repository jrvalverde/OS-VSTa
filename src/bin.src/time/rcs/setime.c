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
@setime command
@


1.1
log
@Initial revision
@
text
@/*
 * main.c
 *	Main routine
 */
#include <sys/types.h>

extern ulong read_time(void);
extern void prtime(void);

main()
{
	ulong clock;
	struct time t;

	clock = read_time();
	if (clock == 0L) {
		printf("Can't read clock; system time not changed.\n");
		exit(1);
	}
	t.t_sec = clock;
	t.t_usec = 0L;
	if (time_set(&t) < 0) {
		perror("time_set");
		exit(1);
	}
	prtime();
	return(0);
}
@
