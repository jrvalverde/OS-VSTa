head	1.2;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.2
date	94.09.23.20.39.10;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.15.05;	author vandys;	state Exp;
branches;
next	;


desc
@Dumb name, but compatible with BSD/Mach/etc.  Definitions for
mmap(2) interface.
@


1.2
log
@Add mapping of files
@
text
@#ifndef _MMAN_H
#define _MMAN_H
/*
 * mman.h
 *	Definitions for mmap system call
 *
 * This name is inherited from the Mach-ish organization of mmap().
 * VSTa only supports a subset of their interface.
 */
#include <sys/types.h>

/*
 * mmap()
 *	Map stuff
 */
extern void *mmap(void *vaddr, ulong len, int prot, int flags,
	int fd, ulong offset);
extern int munmap(void *vaddr, ulong len);
#ifdef KERNEL
extern void *add_map(struct vas *,
	struct portref *, void *, ulong, ulong, int);
#endif /* KERNEL */

/*
 * Bits for prot
 */
#define PROT_EXEC (1)
#define PROT_READ (2)
#define PROT_WRITE (4)

/*
 * Bits for flags
 */
#define MAP_ANON (1)		/* fd not used--anonymous memory */
#define MAP_FILE (2)		/* map from fd */
#define MAP_FIXED (4)		/* must use vaddr or fail */
#define MAP_PRIVATE (8)		/* copy-on-write */
#define MAP_SHARED (16)		/* share same memory */
#define MAP_PHYS (32)		/* talk to physical memory */

#endif /* _MMAN_H */
@


1.1
log
@Initial revision
@
text
@d19 4
@
