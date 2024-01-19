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
date	93.06.30.19.52.49;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.31.04.35.07;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.17.09;	author vandys;	state Exp;
branches;
next	;


desc
@Interface to QIO subsystem
@


1.3
log
@GCC warning cleanup
@
text
@#ifndef _QIO_H
#define _QIO_H
/*
 * qio.h
 *	Data structures and routine definitions for kernel asynch I/O
 */
#include <sys/types.h>

/*
 * One of these describes a queued I/O
 */
struct qio {
	struct portref *q_port;	/* Port for operation */
	int q_op;		/* FS_READ/FS_WRITE */
	struct pset *q_pset;	/* Page set page within */
	struct perpage *q_pp;	/* Per-page info on page */
	off_t q_off;		/* Byte offset in q_port */
	uint q_cnt;		/* Byte count of operation */
	voidfun q_iodone;	/* Function to call on I/O done */
	struct qio *q_next;	/* For linking qios */
};

/*
 * Queue an I/O to the I/O daemon
 */
extern void qio(struct qio *);

/*
 * Allocate a qio structure
 */
extern struct qio *alloc_qio(void);

#endif /* _QIO_H */
@


1.2
log
@Fix prototypes
@
text
@d29 1
a29 1
 * Allocate/free a qio structure
a31 1
extern void free_qio(struct qio *);
@


1.1
log
@Initial revision
@
text
@d26 1
a26 1
void qio(struct qio *);
d29 1
a29 1
 * Allocate a qio structure
d31 2
a32 1
struct qio *alloc_qio(void);
@
