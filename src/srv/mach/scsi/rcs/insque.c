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
 * insque.c - for those runtimes that don't include insque/remque.
 */
#include "insque.h"

#if	defined(__TURBOC__) || defined(__VSTA__)
void	insque(elem, pred)
struct	q_header *elem, *pred;
{
	elem->q_forw = pred->q_forw;
	elem->q_back = pred;
	pred->q_forw->q_back = elem;
	pred->q_forw = elem;
}

void	remque(elem)
struct	q_header *elem;
{
	elem->q_forw->q_back = elem->q_back;
	elem->q_back->q_forw = elem->q_forw;
}
#endif	/*__TURBOC__*/

@
