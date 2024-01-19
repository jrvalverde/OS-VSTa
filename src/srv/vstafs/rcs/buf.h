head	1.8;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.7
	V1_1:1.7;
locks; strict;
comment	@ * @;


1.8
date	94.11.28.04.19.58;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.09.11.19.06.30;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.30.22.33.26;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.18.48.31;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.28.14.15.40;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.27.13.42.15;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.08.59.12;	author vandys;	state Exp;
branches;
next	;


desc
@Buffering definitions
@


1.8
log
@Add special handling for 1st sector versus whole buffer.
Helps for case of dealing with file info but not whole file
contents (e.g., ls -l and such)
@
text
@#ifndef BUF_H
#define BUF_H
/*
 * buf.h
 *	Definitions for the block buffer code
 */
#include <sys/types.h>
#include "vstafs.h"
#include <llist.h>

/*
 * Description of a particular buffer of data
 */
struct buf {
	struct llist *b_list;	/* Linked under bufhead */
	void *b_data;		/* Actual data */
	daddr_t b_start;	/* Starting sector # */
	uint b_nsec;		/*  ...# SECSZ units contained */
	uint b_flags;		/* Flags */
	uint b_locks;		/* Count of locks on buf */
};

/*
 * Bits in b_flags
 */
#define B_SEC0 0x1		/* 1st sector valid  */
#define B_SECS 0x2		/*  ...rest of sectors valid too */
#define B_DIRTY 0x4		/* Some sector in buffer is dirty */

/*
 * Routines for accessing a buf
 */
extern struct buf *find_buf(daddr_t, uint);
extern void *index_buf(struct buf *, uint, uint);
extern void init_buf(void);
extern void dirty_buf(struct buf *);
extern void lock_buf(struct buf *), unlock_buf(struct buf *);
extern void sync_buf(struct buf *);
extern int resize_buf(daddr_t, uint, int);
extern void inval_buf(daddr_t, uint);
extern void sync(void);

#endif /* BUF_H */
@


1.7
log
@Source reorg
@
text
@d26 3
a28 1
#define B_DIRTY 2		/* Buffer needs to be written to disk */
@


1.6
log
@Rename to resize_buf
@
text
@d8 2
a9 2
#include <vstafs/vstafs.h>
#include <lib/llist.h>
@


1.5
log
@Add sync()
@
text
@d37 1
a37 1
extern int extend_buf(daddr_t, uint, int);
@


1.4
log
@Allow locking to next
@
text
@d39 1
@


1.3
log
@Further file creation/deletion stuff
@
text
@d20 1
a25 1
#define B_BUSY 1		/* Buffer is in use */
@


1.2
log
@Convert buffer resize to use only block addresses.  Do rest of
read/write support.
@
text
@d38 1
@


1.1
log
@Initial revision
@
text
@d37 1
a37 1
extern int extend_buf(struct buf *, ulong, int);
@
