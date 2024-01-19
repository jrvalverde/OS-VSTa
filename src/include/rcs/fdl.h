head	1.6;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.6
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.6
date	93.08.31.00.07.27;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.16.19.12.46;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.11.19.18.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.26.18.46.02;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.21.23.23;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.48.19;	author vandys;	state Exp;
branches;
next	;


desc
@File Descriptor Layer definitions
@


1.6
log
@Fix definitions for seek to map onto standard off_t
@
text
@#ifndef _FDL_H
#define _FDL_H
/*
 * fdl.h
 *	Externally-visible definitions for File Descriptor Layer
 */
#include <sys/types.h>

typedef off_t (*__offt_fun)();

/*
 * Per-connection state information
 */
struct port {
	port_t p_port;	/* Port connection # */
	void *p_data;	/* Per-port-type state */
	intfun p_read,	/* Read/write/etc. functions */
		p_write,
		p_close;
	__offt_fun p_seek;
	uint p_refs;	/* # FD's mapping to this port # */
	ulong p_pos;	/* Absolute byte offset in file */
};

/*
 * Internal routines
 */
extern uint __fdl_size(void);
extern void __fdl_save(char *, ulong);
extern char *__fdl_restore(char *);
extern port_t __fd_port(int);
extern int __fd_alloc(port_t);
extern struct port *__port(int);

#endif /* _FDL_H */
@


1.5
log
@Add prototypes for more internal functions
@
text
@d9 2
d19 2
a20 2
		p_close,
		p_seek;
@


1.4
log
@Add position to state of FDL.  See fdl.c for commentary.
@
text
@d29 3
@


1.3
log
@Fix prototypes, tweak return value for fdl_restore()
@
text
@d20 1
@


1.2
log
@Add prototypes for our internal functions
@
text
@d25 3
a27 2
extern uint __fd_size(void);
extern void __fd_save(char *, ulong), __fd_restore(char *);
@


1.1
log
@Initial revision
@
text
@d22 6
@
