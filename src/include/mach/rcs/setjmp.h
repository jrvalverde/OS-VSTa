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
date	93.01.29.16.07.22;	author vandys;	state Exp;
branches;
next	;


desc
@Definition of the jmp_buf as handled by locore.s in kernel and
setjmp.s in user mode
@


1.1
log
@Initial revision
@
text
@#ifndef _MACHSETJMP_H
#define _MACHSETJMP_H
/*
 * setjmp.h
 *	The context we keep to allow longjmp()'ing
 */
#include <sys/types.h>

/*
 * While not apparent, the layout matches a pushal instruction,
 * with the addition of EIP at the bottom of the "stack".
 */
typedef struct {
	ulong eip, edi, esi, ebp, esp, ebx, edx, ecx, eax;
} jmp_buf[1];

/*
 * Routines for using this context
 */
extern int setjmp(jmp_buf);
extern void longjmp(jmp_buf, int);

#endif /* _MACHSETJMP_H */
@
