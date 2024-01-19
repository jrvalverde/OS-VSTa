head	1.6;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.6
	V1_1:1.6
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.6
date	93.04.14.01.08.10;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.12.23.26.10;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.24.00.35.38;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.20.00.19.19;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.03.23.14.32;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.16.13;	author vandys;	state Exp;
branches;
next	;


desc
@Permission and protection definitions
@


1.6
log
@convert to system unsigned types
@
text
@/*
 * perm.h
 *	Definitions for permission/protection structures
 */
#ifndef _PERM_H
#define _PERM_H
#include <sys/types.h>
#include <sys/param.h>

struct perm {
	uchar perm_len;		/* # slots valid */
	uchar perm_id[PERMLEN];	/* Permission values */
	ulong perm_uid;		/* UID for this ability */
};

struct prot {
	uchar prot_len;		/* # slots valid */
	uchar prot_default;	/* Capabilities available to all */
	uchar prot_id[PERMLEN];	/* Permission values */
	uchar			/* Capabilities granted */
		prot_bits[PERMLEN];
};

/*
 * Macros for fiddling perms
 */
#define PERM_ACTIVE(p) ((p)->perm_len < PERMLEN)
#define PERM_DISABLE(p) ((p)->perm_len |= 0x80)
#define PERM_DISABLED(p) ((p)->perm_len & 0x80)
#define PERM_ENABLE(p) ((p)->perm_len &= ~0x80)
#define PERM_NULL(p) ((p)->perm_len = PERMLEN)
#define PERM_LEN(p) (((p)->perm_len & ~0x80) % PERMLEN)

/*
 * Prototypes
 */
extern int perm_calc(struct perm *, int, struct prot *);
extern void zero_ids(struct perm *, int);
extern int perm_dominates(struct perm *, struct perm *);
extern int perm_ctl(int, struct perm *, struct perm *);

#endif /* _PERM_H */
@


1.5
log
@Add UID tag for abilities
@
text
@d7 1
d11 3
a13 3
	unsigned char perm_len;		/* # slots valid */
	unsigned char perm_id[PERMLEN];	/* Permission values */
	unsigned long perm_uid;		/* UID for this ability */
d17 4
a20 4
	unsigned char prot_len;		/* # slots valid */
	unsigned char prot_default;	/* Capabilities available to all */
	unsigned char prot_id[PERMLEN];	/* Permission values */
	unsigned char			/* Capabilities granted */
@


1.4
log
@PERM_LEN has to be right for PERM_DISABLED perm_len values
@
text
@d12 1
@


1.3
log
@Add some macros and some prototypes
@
text
@d30 1
a30 1
#define PERM_LEN(p) ((p)->perm_len % PERMLEN)
@


1.2
log
@Add prototypes for perm_calc() and zero_ids()
@
text
@d22 13
a34 1
#ifdef KERNEL
d37 2
a38 1
#endif
@


1.1
log
@Initial revision
@
text
@d21 6
@
