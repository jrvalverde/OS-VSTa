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
@Kill process
@


1.1
log
@Initial revision
@
text
@#include <sys/fs.h>

main(int argc, char **argv)
{
	int x, pid;
	char *event = EKILL;

	for (x = 1; x < argc; ++x) {
		if (argv[x][0] == '-') {
			event = argv[x]+1;
		} else {
			pid = atoi(argv[1]);
			notify(pid, 0, event);
		}
	}
	return(0);
}
@
