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
date	93.01.29.16.19.15;	author vandys;	state Exp;
branches;
next	;


desc
@Write to console endlessly
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
	struct msg m;

	x = msg_connect(PORT_CONS, ACC_WRITE);
	for (;;) {
		m.m_op = FS_WRITE;
		m.m_nseg = 1;
		m.m_buf = msg;
		m.m_arg = m.m_buflen = strlen(msg);
		m.m_arg1 = 0;
		msg_send(x, &m);
	}
}
@
