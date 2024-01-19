head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@ * @;


1.1
date	94.03.01.17.19.11;	author vandys;	state Exp;
branches;
next	;


desc
@Archive header for "ar" program
@


1.1
log
@Initial revision
@
text
@#ifndef _AR_H
#define _AR_H
/*
 * ar.h
 *	Header format of an ar(1) archive
 */
#define ARMAG "!<arch>\n"
#define SARMAG 8
#define RANLIBMAG "__.SYMDEF"	/* XXX */

#define ARFMAG "`\n"

struct ar_hdr {
	char ar_name[16];
	char ar_date[12];
	char ar_uid[6];
	char ar_gid[6];
	char ar_mode[8];
	char ar_size[10];
	char ar_fmag[2];
};

#endif /* _AR_H */
@
