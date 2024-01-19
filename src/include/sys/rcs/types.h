head	1.9;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.9
date	95.01.10.05.14.50;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.12.21.05.32.07;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.09.23.20.39.51;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.05.30.21.32.28;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.03.18.17.56.12;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.03.04.02.02.58;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.06.30.19.52.57;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.09.17.11.04;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.17.45;	author vandys;	state Exp;
branches;
next	;


desc
@Some common type declarations
@


1.9
log
@Move uid_t and gid_t to system types.
@
text
@#ifndef _TYPES_H
#define _TYPES_H
/*
 * types.h
 *	Central repository for globally known types
 */
typedef unsigned int uint;
typedef unsigned int uint_t;
typedef unsigned int u_int;
typedef unsigned short ushort;
typedef unsigned short ushort_t;
typedef unsigned short u_short;
typedef unsigned char uchar;
typedef unsigned char uchar_t;
typedef unsigned char u_char;
typedef unsigned long ulong;
typedef unsigned long ulong_t;
typedef unsigned long u_long;
typedef unsigned long off_t;
typedef unsigned long size_t;
struct time {
	long t_sec;
	long t_usec;
};
typedef int port_t;
typedef int port_name;
typedef long pid_t;
typedef void (*voidfun)();
typedef int (*intfun)();
typedef unsigned long uid_t, gid_t;

/*
 * For talking about a structure without knowing its definition
 */
#define STRUCT_REF(type) typedef struct type __##type##_

/*
 * These are structures who are accessed in this way
 */
STRUCT_REF(proc);
STRUCT_REF(thread);
STRUCT_REF(vas);
STRUCT_REF(pset);
STRUCT_REF(pview);
STRUCT_REF(prot);
STRUCT_REF(perm);
STRUCT_REF(port);
STRUCT_REF(portref);

#endif /* _TYPES_H */
@


1.8
log
@Allow signed time values.  Dave's got this crazy thing for
the 1980's being a negative time.  Come to think of it, it
*was* rather a negative time.
@
text
@d30 1
@


1.7
log
@Add some more opaque types
@
text
@d22 2
a23 2
	ulong t_sec;
	ulong t_usec;
@


1.6
log
@Move all struct refs here, delete extraneous semicolon
@
text
@d46 2
@


1.5
log
@Add many more names for the same old types
@
text
@d34 12
a45 1
#define STRUCT_REF(type) typedef struct type __##type##_;
@


1.4
log
@Fix warnings, prepare for -Wall
@
text
@d8 2
d11 2
d14 2
d17 2
@


1.3
log
@Add size_t type, for use of libc
@
text
@d23 5
@


1.2
log
@Add pid_t
@
text
@d12 1
@


1.1
log
@Initial revision
@
text
@d18 1
@
