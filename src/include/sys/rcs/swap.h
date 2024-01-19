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
date	95.01.10.05.15.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.13.08;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for swap data structures and messagtes
@


1.2
log
@Use path_open instead of distinct port/path construct
@
text
@#ifndef _SWAP_H
#define _SWAP_H
/*
 * swap.h
 *	Data structures for swap manager
 *
 * The swap task simply manages one or more swap partitions,
 *
 * This manager is currently single threaded.  It will be appropriate
 * in the future to allow enough threads to queue I/O to each device
 * on which swap blocks reside in parallel.
 *
 * Offsets to this manager are in page-sized blocks.  Since byte offsets
 * are not needed, this allows us to potentially support very large
 * swap spaces.
 */
#include <sys/types.h>
#include <sys/fs.h>

/*
 * Extra functions this manager provides
 */
#define SWAP_ALLOC (301)	/* Allocate swap space */
#define SWAP_FREE (302)		/* Free previously allocated space */
#define SWAP_ADD (303)		/* Add space for swap */

/*
 * Format of message which accompanies SWAP_ADD
 */
struct swapadd {
	char s_path[32];	/* port:path name format */
	ulong s_off;		/* Offset in port, in blocks */
	ulong s_len;		/* # blocks */
};

/*
 * Our per-open-file data structure
 */
struct file {
	ulong f_pos;	/* Current file offset, in blocks */
	int f_perms;	/* Mask of operations allowed */
};

/*
 * Map of swap block #'s to device/offset.  When s_port is 0, this
 * represents a block server which has died; all future requests
 * to this range must be failed with EIO.
 */
struct swapmap {
	ulong s_block;		/* Swap block # and length */
	ulong s_len;
	port_t s_port;		/* Port blocks served via */
	ulong s_off;		/*  ...offset on that port */
};

#endif /* _SWAP_H */
@


1.1
log
@Initial revision
@
text
@d31 1
a31 1
	port_name s_port;	/* Port which serves */
a33 1
	char s_path[16];	/* Name within port */
@
