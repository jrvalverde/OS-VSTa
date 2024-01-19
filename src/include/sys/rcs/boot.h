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
date	93.01.29.16.13.40;	author vandys;	state Exp;
branches;
next	;


desc
@Data structure needed during bootup
@


1.1
log
@Initial revision
@
text
@#ifndef _BOOT_H
#define _BOOT_H
/*
 * boot.h
 *	Cruft needed only during boot
 */
#include <sys/types.h>

/*
 * This describes a boot task which was loaded along with the kernel
 * during IPL.  The machine-dependent code figures out where they
 * are, and fills in an array of these to describe them in a portable
 * way.
 */
struct boot_task {
	uint b_pc;		/* Starting program counter */
	void *b_textaddr;	/* Address of text */
	uint b_text;		/* # pages of text */
	void *b_dataaddr;	/* Address of data */
	uint b_data;		/* # pages data */
	uint b_pfn;		/* Physical address of task */
};
#ifdef KERNEL
extern struct boot_task *boot_tasks;
extern uint nboot_task;
#endif

#endif /* _BOOT_H */
@
