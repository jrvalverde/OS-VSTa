head	1.13;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.5
	V1_1:1.5
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.13
date	94.10.04.20.27.34;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.09.30.17.39.47;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.09.23.20.39.27;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.07.10.19.32.46;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.05.30.21.31.32;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.03.28.23.50.34;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.03.23.21.53.25;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.15.22.06.27;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.20.21.24.30;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.19.21.40.16;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.09.17.10.39;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.17.20.20.43;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.14.10;	author vandys;	state Exp;
branches;
next	;


desc
@Generic filesystem messages and definitions
@


1.13
log
@Merge Dave Hudson's changes for unaligned block I/O support
and fixes for errno emulation.
@
text
@#ifndef _FS_H
#define _FS_H
/*
 * Custom message types for a generic filesystem
 */
#include <sys/msg.h>

/*
 * FS operations which should basically be offered by all
 * servers.
 */
#define FS_OPEN 100
#define FS_READ 101
#define FS_SEEK 102
#define FS_WRITE 103
#define FS_REMOVE 104
#define FS_STAT 105
#define FS_WSTAT 106

/*
 * The following are recommended, but not required
 */
#define FS_ABSREAD 150		/* Mostly for swap/paging operations */
#define FS_ABSWRITE 151
#define FS_FID 152		/* For caching of mapped files */
#define FS_RENAME 153		/* Renaming of files */

/*
 * How to ask if an operation is a bulk read/write
 */
#define FS_RW(op) (((op) == FS_READ) || ((op) == FS_WRITE) || \
	((op) == FS_ABSREAD) || ((op) == FS_ABSWRITE))

/*
 * Access modes
 */
#define ACC_EXEC	0x1
#define ACC_WRITE	0x2
#define ACC_READ	0x4
#define ACC_SYM		0x8
#define ACC_CREATE	0x10
#define ACC_DIR		0x20
#define ACC_CHMOD	0x40

/*
 * Standard error strings
 */
#define EPERM "perm"
#define ENOENT "no file"
#define ESRCH "no entry"
#define EINTR "intr"
#define EIO "io err"
#define ENXIO "no io"
#define E2BIG "too big"
#define ENOEXEC "exec fmt"
#define EBADF "bad file"
#define ECHILD "no child"
#define EAGAIN "again"
#define EWOULDBLOCK EAGAIN
#define ENOMEM "no mem"
#define EACCES "access"
#define EFAULT "fault"
#define ENOTBLK "not blk dev"
#define EBUSY "busy"
#define EEXIST "exists"
#define ENODEV "not dev"
#define ENOTDIR "not dir"
#define EISDIR "is dir"
#define EINVAL "invalid"
#define ENFILE "file tab ovfl"
#define EMFILE "too many files"
#define ENOTTY "not tty"
#define ETXTBSY "txt file busy"
#define EFBIG "file too large"
#define ENOSPC "no space"
#define ESPIPE "ill seek"
#define EROFS "RO fs"
#define EMLINK "too many links"
#define EPIPE "broken pipe"
#define EDOM "math domain"
#define ERANGE "math range"
#define EMATH "math"
#define EILL "ill instr"
#define EKILL "kill"
#define EXDEV "cross dev"
#define EISDIR "is dir"
#define EBALIGN "blk align"
#define ESYMLINK "symlink"
#define ELOOP "symlink loop"

/*
 * A stat of an entry returns a set of strings with newlines
 * between them.  Each string has the format "key=value".  The
 * following keys and their meaning are required by any file
 * server.
 *
 * perm		List of IDs on protection label
 * acc		List of bits (see ACC_* above) corresponding to perm
 * size		# bytes in file
 * owner	ID of creator of file
 * inode	Unique index number for file
 *
 * Not required, but with a common interpretation:
 * dev		Block device's port number
 * ctime	Create/Modify/Access times as 14-digit decimal number
 * mtime
 * atime
 * gen		Access generation--for protecting TTY's between sessions
 * block	Bytes in a block used on device
 * blocks	Actual blocks used by entry
 */

/*
 * Function prototypes for the VSTa stat mechanism
 */
extern char *rstat(port_t fd, char *field);
extern int wstat(port_t fd, char *field);

/*
 * Additional function used to help servers process the general stat fields
 */
extern int do_wstat(struct msg *m, struct prot *prot,
		    int acc, char **fieldp, char **valp);

#endif /* _FS_H */
@


1.12
log
@Dev stuff moved elsewhere, according to Dave Hudson
@
text
@d87 1
@


1.11
log
@Add symlinks
@
text
@a28 7
 * Messages to facilitate handling of device specific commands and
 * automatic error/fault recovery
 */
#define DEV_READ 200
#define DEV_WRITE 201

/*
@


1.10
log
@Add blocks value for # blocks used by a file
@
text
@d44 7
a50 7
#define ACC_READ 0x4
#define ACC_WRITE 0x2
#define ACC_EXEC 0x1
/* #define ACC_TRUNC 0x8	XXX does ACC_CREATE cover this? */
#define ACC_CREATE 0x10
#define ACC_DIR 0x20
#define ACC_CHMOD 0x40
d94 2
@


1.9
log
@Add protos for VSTa messages related to FS_STAT/FS_WSTAT
@
text
@d113 2
@


1.8
log
@Improved errno emulation
@
text
@d115 12
@


1.7
log
@Add EISDIR
@
text
@d29 7
d56 1
d58 3
a60 1
#define EINVAL "invalid"
d62 5
d68 3
d72 10
d83 6
a88 6
#define ENOTDIR "not dir"
#define EEXIST "exists"
#define EIO "io err"
#define ENXIO "no io"
#define EFAULT "fault"
#define EINTR "intr"
a91 2
#define EBADF "bad file"
#define EAGAIN "again"
@


1.6
log
@Add rename() support
@
text
@d67 1
@


1.5
log
@Add EAGAIN to tell'em "try again"
@
text
@d26 1
d66 1
@


1.4
log
@Add FS_FID for caching of mapped files
@
text
@d64 1
@


1.3
log
@Add EBADF
@
text
@d8 4
d21 1
a21 2
 * The following are recommended, but not required.  They are
 * currently only needed to support swap operations.
d23 1
a23 1
#define FS_ABSREAD 150
d25 1
@


1.2
log
@Add "exists" error for trying to create an existing directory
@
text
@d59 1
@


1.1
log
@Initial revision
@
text
@d51 1
@
