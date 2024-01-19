head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.10.05.19.40.46;	author vandys;	state Exp;
branches;
next	;


desc
@Initial RCS check-in of Mike Larson's SCSI subsystem
@


1.1
log
@Initial revision
@
text
@/*
 * insque.h - C library insque()/remque() definitions.
 */

#ifndef	__INSQUE_H__
#define	__INSQUE_H__

struct	q_header {
	struct	q_header *q_forw, *q_back;
};

struct	q_elem {
	struct	q_elem *q_forw, *q_back;
	char	q_data[1];
};

#ifdef	__STDC__
void	insque(struct q_header *element, struct q_header *pred);
void	remque(struct q_header *element);
#endif

#endif	/*__INSQUE_H__*/

@
