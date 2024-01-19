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
date	93.02.23.19.29.16;	author vandys;	state Exp;
branches;
next	;


desc
@Test of timing intervals
@


1.1
log
@Initial revision
@
text
@#include <sys/ports.h>
#include <sys/fs.h>

static void
tick()
{
	static char msg[] = " tick\n";

	for (;;) {
		sleep(1);
		write(1, msg, sizeof(msg)-1);
	}
}

main()
{
	port_t kbd, scrn;
	static char msg[] = "Tock\n";

	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);

	tfork(tick);
	for (;;) {
		sleep(2);
		write(1, msg, sizeof(msg)-1);
	}
}
@
