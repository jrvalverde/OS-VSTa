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
date	93.01.29.16.19.43;	author vandys;	state Exp;
branches;
next	;


desc
@Read from keybd in cooked mode, write to console
@


1.1
log
@Initial revision
@
text
@#include <sys/ports.h>
#include <sys/fs.h>
#include <stdio.h>

main()
{
	int scrn, kbd;
	int x;
	char buf[128];

	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	printf("Hello, world.\n");
	for (;;) {
		if (gets(buf) == 0) {
			printf("Read failed\n");
			continue;
		}
		printf("Got '%s'\n", buf);
	}
}
@
