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
date	93.01.29.15.41.28;	author vandys;	state Exp;
branches;
next	;


desc
@Mapping of 32-bit linear to 16-bit Intel segments
@


1.1
log
@Initial revision
@
text
@/*
 * seg.c
 *	Utilities for straddling the 16- and 32-bit worlds
 */
#include <stdlib.h>
#include <memory.h>
#include "boot.h"

/*
 * lptr()
 *	Convert long linear pointer to a usable long segmented pointer
 */
void *
lptr(ulong l)
{
	ulong l2;

	l2 = l >> 4;
	if (l2 & ~0xFFFF) {
		printf("Error: memory overflow");
		exit(1);
	}
	return (void *)((l2 << 16) | (l & 0xF));
}

/*
 * linptr()
 *	Convert segment pointer to linear
 */
ulong
linptr(void *p)
{
	ulong l;

	l = (ulong)p;
	return (
		(l & 0xFFFF) +		/* Offset */
		((l >> 16) << 4));	/* Seg, converted to bytes */
}

/*
 * lread()
 *	Do reads into a long pointer space
 *
 * Breaks I/O into chunks which DOS will accept.  Aborts program on
 * I/O error.
 */
void
lread(FILE *fp, ulong paddr, ulong nbyte)
{
	char *p;
	int x, count;

	while (nbyte > 0L) {
		p = lptr(paddr);
		if (nbyte > 16*K) {
			count = 16*K;
		} else {
			count = nbyte;
		}
		x = fread(p, sizeof(char), count, fp);
		if (x != count) {
			printf("I/O error during read: want %d got %d\n",
				count, x);
			exit(1);
		}
		nbyte -= count;
		paddr += count;
	}
}

/*
 * lbzero()
 *	Zero memory using linear pointer
 */
void
lbzero(ulong paddr, ulong nbyte)
{
	uint count;
	char *p;

	while (nbyte > 0L) {
		if (nbyte > 16*K) {
			count = 16*K;
		} else {
			count = nbyte;
		}
		p = lptr(paddr);
		memset(p, '\0', count);
		nbyte -= count;
		paddr += count;
	}
}

/*
 * lbcopy()
 *	Copy memory using linear pointers
 */
void
lbcopy(ulong src, ulong dest, ulong nbyte)
{
	uint count;
	char *srcp, *destp;

	while (nbyte > 0L) {
		if (nbyte > 16*K) {
			count = 16*K;
		} else {
			count = nbyte;
		}
		srcp = lptr(src);
		destp = lptr(dest);
		memcpy(destp, srcp, count);
		src += count;
		dest += count;
		nbyte -= count;
	}
}
@
