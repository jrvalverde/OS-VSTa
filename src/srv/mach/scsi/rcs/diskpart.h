head	1.1;
access;
symbols;
locks; strict;
comment	@ * @;


1.1
date	94.10.05.19.40.45;	author vandys;	state Exp;
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
 * diskpart.h - disk partition definitions (adapted from wd/wd.h).
 */

#ifndef	__DISKPART_H__
#define	__DISKPART_H__
/*
 * Shape of a partition
 */
struct part {
	char p_name[16];	/* Symbolic name */
	ulong p_off;		/* Sector offset */
	ulong p_len;		/*  ...length */
	int p_val;		/* Valid slot? */
	struct prot		/* Protection for partition */
		p_prot;
};

#endif	/*__DISKPART_H__*/
@
