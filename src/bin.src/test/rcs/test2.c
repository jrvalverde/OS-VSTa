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
date	93.01.29.16.19.27;	author vandys;	state Exp;
branches;
next	;


desc
@Write to console endlessly using FDL
@


1.1
log
@Initial revision
@
text
@#include <sys/ports.h>
#include <sys/fs.h>

main()
{
	int x;
	static char msg[] = "Hello, world.\n";

	x = msg_connect(PORT_CONS, ACC_WRITE);
	x = __fd_alloc(x);
	for (;;) {
		write(x, msg, sizeof(msg)-1);
	}
}
@
