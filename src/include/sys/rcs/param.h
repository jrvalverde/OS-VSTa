head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.3
date	93.02.19.17.59.35;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.15.13.49;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.15.53;	author vandys;	state Exp;
branches;
next	;


desc
@Portable parameters set for the kernel
@


1.3
log
@Get rid of extraneous semicolon
@
text
@/*
 * param.h
 *	Global parameters
 */
#ifndef _PARAM_H
#define _PARAM_H
#include <mach/param.h>

#define NAMESZ 16	/* Max name length for various system objects */
#define PERMLEN 7	/* Max levels of permission discrimination */
#define PROCPERMS 6	/* # permissions per process */
#define PROCPORTS 4	/* Max ports offered by process */
#define PROCOPENS 32	/* Max port references from process */
#define MAXDIR 64	/* Max # bytes in dirread()-format message */
#define MAXSTAT 256	/*  ...in stat() response message */
#define MAXQIO 6	/* Max queued I/O's at a time */
#define VMAPSIZE 80	/* Size of rmap for utility virtual memory */
#define EVLEN 16	/* Max chars in an event string */
#define ERRLEN 16	/*  ...in error string */
#define MSGSEGS 4	/* Max # segments to a message */
			/* Does not include m_seg0, so add one for kernel */
#define NPROC 64	/* Number of threads on system */
#define KSTACK_SIZE \
	(NBPG)		/* Size of kernel stack */
#define MAX_WIRED (4)	/* Max # pages wired for DMA at once */

/*
 * Each thread starts with a stack of size UMINSTACK.  For the special
 * case of a single-threaded process, the thread may fault once below
 * its current stack and get another UMAXSTACK size extension.
 */
#define UMINSTACK (16*NBPG)
#define UMAXSTACK (512*NBPG)

#define roundup(val, units) \
	((((unsigned long)(val) + ((units) - 1)) / (units)) * (units))

#endif /* _PARAM_H */
@


1.2
log
@Add new parameter for how many pages to wire at once
@
text
@d36 1
a36 1
	((((unsigned long)(val) + ((units) - 1)) / (units)) * (units));
@


1.1
log
@Initial revision
@
text
@d25 1
@
