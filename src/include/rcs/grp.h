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
date	95.01.10.05.13.09;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.06.30.21.25.30;	author vandys;	state Exp;
branches;
next	;


desc
@Group file handling
@


1.2
log
@Move gid_t to system types file
@
text
@#ifndef _GRP_H
#define _GRP_H
/*
 * grp.h
 *	"groups", ala VSTa
 *
 * Much of the POSIX group concept is missing; we emulate
 * what we can.
 */
#include <sys/types.h>

/*
 * Group information
 */
struct group {
	char *gr_name;		/* Name of group */
	gid_t gr_gid;		/* GID */
	char **gr_mem;		/* Members (not tabulated) */
	char *gr_ids;		/* List of IDs granted this group */
};

/*
 * Procedures
 */
extern struct group *getgrgid(gid_t),
	*getgrnam(char *);

#endif /* _GRP_H */
@


1.1
log
@Initial revision
@
text
@d10 1
a10 5

/*
 * Numeric group ID
 */
typedef unsigned long gid_t;
@
