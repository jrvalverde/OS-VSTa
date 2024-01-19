head	1.7;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.5
	V1_1:1.5
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.7
date	94.05.30.21.29.29;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.05.24.17.09.33;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.23.19.48.14;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.26.15.42.58;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.10.19.04.01;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.42.55;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.11.40;	author vandys;	state Exp;
branches;
next	;


desc
@Mapping of namer operations into communications with namer server
@


1.7
log
@Fix tricky edge case; boot servers *will* get their namer connection
on port #0
@
text
@/*
 * namer.c
 *	Stubs for talking to namer
 *
 * The namer routines are not syscalls nor even central system
 * routines.  They merely encapsulate a message-based conversation
 * with a namer daemon.  It could be written as a set of normal
 * filesystem operations, except that we prefer to not assume the
 * organization of the user's name space.
 */
#include <sys/fs.h>
#include <sys/ports.h>
#include <std.h>

/*
 * namer_register()
 *	Register the given port number under the name
 */
namer_register(char *buf, port_name uport)
{
	char *p;
	char numbuf[8], wdbuf[64];
	int x;
	port_t port;
	struct msg m;

	/*
	 * Skip leading '/'
	 */
	while (*buf == '/') {
		++buf;
	}

	/*
	 * Connect to name server
	 */
	for (x = 0; x < 5; ++x) {
		port = msg_connect(PORT_NAMER, ACC_READ|ACC_WRITE);
		if (port >= 0) {
			break;
		}
		__msleep(100);
	}
	if (port < 0) {
		return(-1);
	}

	/*
	 * Create the directories leading down to the name
	 */
	while (p = strchr(buf, '/')) {
		/*
		 * Null-terminate name
		 */
		bcopy(buf, wdbuf, p-buf);
		wdbuf[p-buf] = '\0';
		++p;

		/*
		 * Send a creation message
		 */
		m.m_op = FS_OPEN;
		m.m_buf = wdbuf;
		m.m_buflen = strlen(wdbuf)+1;
		m.m_nseg = 1;
		m.m_arg = ACC_CREATE|ACC_DIR|ACC_WRITE;
		m.m_arg1 = 0;
		if (msg_send(port, &m) < 0) {
			msg_disconnect(port);
			return(-1);
		}

		/*
		 * Advance to next element
		 */
		buf = p;
	}

	/*
	 * Final element--should be file in the current directory
	 */
	m.m_op = FS_OPEN;
	m.m_buf = buf;
	m.m_buflen = strlen(buf)+1;
	m.m_nseg = 1;
	m.m_arg = ACC_CREATE|ACC_WRITE;
	m.m_arg1 = 0;
	if (msg_send(port, &m) < 0) {
		msg_disconnect(port);
		return(-1);
	}

	/*
	 * Write our port number here
	 */
	sprintf(numbuf, "%d\n", uport);
	m.m_op = FS_WRITE;
	m.m_buf = numbuf;
	m.m_arg = m.m_buflen = strlen(numbuf);
	m.m_arg1 = 0;
	x = msg_send(port, &m);
	msg_disconnect(port);
	return(x);
}

/*
 * namer_find()
 *	Given name, look up a port
 */
port_name
namer_find(char *buf)
{
	char *p;
	int x;
	port_t port;
	struct msg m;
	char numbuf[8], wdbuf[64];

	/*
	 * Skip leading '/'
	 */
	while (*buf == '/') {
		++buf;
	}

	/*
	 * Connect to name server
	 */
	port = msg_connect(PORT_NAMER, ACC_READ);
	if (port < 0) {
		return(-1);
	}

	/*
	 * Search the directories leading down to the name
	 */
	do {
		p = strchr(buf, '/');
		if (p) {
			/*
			 * Null-terminate name
			 */
			bcopy(buf, wdbuf, p-buf);
			wdbuf[p-buf] = '\0';
			++p;
		} else {
			strcpy(wdbuf, buf);
		}

		/*
		 * Send an open message
		 */
		m.m_op = FS_OPEN;
		m.m_buf = wdbuf;
		m.m_buflen = strlen(wdbuf)+1;
		m.m_nseg = 1;
		m.m_arg = ACC_DIR;
		m.m_arg1 = 0;
		if (msg_send(port, &m) < 0) {
			msg_disconnect(port);
			return(-1);
		}

		/*
		 * Advance to next element
		 */
		buf = p;
	} while (p);

	/*
	 * Read our port number from here
	 */
	m.m_op = FS_READ|M_READ;
	m.m_buf = numbuf;
	m.m_arg = m.m_buflen = sizeof(numbuf)-1;
	m.m_nseg = 1;
	m.m_arg1 = 0;
	x = msg_send(port, &m);
	msg_disconnect(port);
	if (x < 0) {
		return(-1);
	}
	numbuf[x] = '\0';
	return(atoi(numbuf));
}
@


1.6
log
@Add retry/sleep loop when registering w. namer
@
text
@d44 1
a44 1
	if (port <= 0) {
@


1.5
log
@Fix up type dec'ls for parameter
@
text
@d37 8
a44 2
	port = msg_connect(PORT_NAMER, ACC_READ|ACC_WRITE);
	if (port < 0) {
@


1.4
log
@Terminate string at indicated length
@
text
@d19 1
a19 1
namer_register(char *buf, port_t uport)
@


1.3
log
@Fix name lookup loop for namer_find()
@
text
@d177 1
a177 1
	numbuf[sizeof(numbuf)-1] = '\0';
@


1.2
log
@Implement namer library
@
text
@d139 1
@


1.1
log
@Initial revision
@
text
@d7 3
a9 3
 * with a namer daemon.  So this file should grow to be some code
 * for connecting to a well-known port number, and talking with
 * a certain format of messages.
d11 3
a13 1
#include <sys/msg.h>
d15 164
a178 2
port_name namer_find(char *class, int flags) { }
int namer_register(char *class, port_t port) { }
@
