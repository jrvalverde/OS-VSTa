head	1.3;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.05.30.04.04.43;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.04.09.03.31.58;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.29.24;	author vandys;	state Exp;
branches;
next	;


desc
@Machine defs
@


1.3
log
@Add PS2, convert to syslog, convert to RS-232 server
@
text
@#ifndef __MACHINE_H__
#define __MACHINE_H__

static inline unsigned int 
set_semaphore(volatile unsigned int * p, unsigned int newval)
{
	unsigned int semval = newval;
__asm__ __volatile__ ("xchgl %2, %0\n"
			 : /* outputs: semval   */ "=r" (semval)
			 : /* inputs: newval, p */ "0" (semval), "m" (*p)
			);	/* p is a var, containing an address */
	return semval;
}

#endif /* __MACHINE_H__ */
@


1.2
log
@Clean up white space
@
text
@a3 18
/* 
 * These may give a slight performance improvement by avoiding the function
 * call overhead
 */
static inline void outportb(unsigned short port, char value)
{
__asm__ __volatile__ ("outb %%al,%%dx"
                ::"a" ((char) value),"d" ((unsigned short) port));
}

static inline unsigned int inportb(unsigned short port)
{
        unsigned int _v;
__asm__ __volatile__ ("inb %%dx,%%al"
                :"=a" (_v):"d" ((unsigned short) port),"0" (0));
        return _v;
}

@


1.1
log
@Initial revision
@
text
@a33 3



@
