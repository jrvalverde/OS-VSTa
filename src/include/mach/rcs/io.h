head	1.3;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.10.05.17.57.14;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.08.27.00.14.42;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.02.01.23.15.37;	author vandys;	state Exp;
branches;
next	;


desc
@I/O handling functions
@


1.3
log
@Add FPU support
@
text
@#ifndef _MACHIO_H
#define _MACHIO_H
/*
 * io.h
 *	Declarations relating to I/O on the i386
 */

#include <sys/types.h>
 
/*
 * Prototypes for a few short I/O routines in the C library
 */
extern uchar inportb(int port);
extern void outportb(int port, uchar data);
extern ushort inportw();
extern void outportw(int port, ushort data);
extern void repinsw(int port, void *buffer, int count);
extern void repoutsw(int port, void *buffer, int count);
extern int cli(void);
extern void sti(void);
#ifdef KERNEL
extern void set_cr0(ulong), set_cr2(ulong), set_cr3(ulong);
extern ulong get_cr0(void), get_cr2(void), get_cr3(void);
#endif /* KERNEL */

#endif /* _MACHIO_H */
@


1.2
log
@Add in/out I/O words
@
text
@d19 6
@


1.1
log
@Initial revision
@
text
@d15 2
@
