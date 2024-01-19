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
date	95.01.10.05.13.24;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.06.30.21.25.47;	author vandys;	state Exp;
branches;
next	;


desc
@Password filehandling
@


1.2
log
@Move uid_t to system types
@
text
@#ifndef _PWD_H
#define _PWD_H
/*
 * pwd.h
 *	Password file functions
 *
 * This represents a subset of what VSTa puts in the passwd file
 */
#include <sys/types.h>

/*
 * Types
 */
struct passwd {
	char *pw_name;		/* Name of account */
	uid_t pw_uid;		/* UID */
	gid_t pw_gid;		/* GID */
	char *pw_dir;		/* Home dir */
	char *pw_shell;		/* Shell */
};

/*
 * Prototypes
 */
extern struct passwd *getpwuid(uid_t),
	*getpwnam(char *);

#endif /* _PWD_H */
@


1.1
log
@Initial revision
@
text
@d9 1
a9 1
#include <grp.h>	/* for gid_t */
a13 1
typedef unsigned long uid_t;
@
