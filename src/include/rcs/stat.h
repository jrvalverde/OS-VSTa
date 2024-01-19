head	1.9;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.9
date	94.10.23.18.11.13;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.08.03.22.03.54;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.07.10.19.17.20;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.07.10.18.24.44;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.05.30.21.31.00;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.10.19.55.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.02.06.19.38.11;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.05.07.00.08.04;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.48.38;	author vandys;	state Exp;
branches;
next	;


desc
@The stat(2) structure
@


1.9
log
@Fix stat() emulation, add more compat macros
@
text
@#ifndef _STAT_H
#define _STAT_H
/*
 * stat.h
 *	The "stat" structure and stat function prototypes
 *
 * The "stat" structure doesn't exist in VSTa; this structure is mocked up
 * for programs requiring such an interface.  Down with large binary
 * structures!
 */
#include <sys/types.h>
#include <time.h>

typedef ulong mode_t;	/* POSIX types, "A name for everybody" */
typedef ulong nlink_t;
typedef ulong dev_t;
typedef ulong ino_t;

struct	stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	ulong st_uid, st_gid;
	dev_t st_rdev;
	off_t st_size;
	time_t st_atime, st_mtime, st_ctime;
	ulong st_blksize;
	ulong st_blocks;
};

/*
 * major/minor device number emulation - definitions
 */
#define major(dev) (((dev) >> 8) & 0xffffff)
#define minor(dev) ((dev) & 0xff)
#define makedev(maj, min) (((maj) << 8) || ((min) & 0x0ff))

/*
 * POSIX mode definitions
 */
#define S_IFMT		0xF000	/* file type mask */
#define S_IFDIR		0x4000	/* directory */
#define S_IFIFO		0x1000	/* FIFO special */
#define S_IFCHR		0x2000	/* character special */
#define S_IFBLK		0x3000	/* block special */
#define S_IFREG		0x0000	/* no bits--regular file */

#define S_IRWXU		00700	/* owner may read, write and execute */
#define S_IRUSR		00400	/* owner may read */
#define S_IWUSR		00200	/* owner may write */
#define S_IXUSR		00100	/* owner may execute */

#define S_IRWXG		00070	/* group may read, write and execute */
#define S_IRGRP		00040	/* group may read */
#define S_IWGRP		00020	/* group may write */
#define S_IXGRP		00010	/* group may execute */

#define S_IRWXO		00007	/* other may read, write and execute */
#define S_IROTH		00004	/* other may read */
#define S_IWOTH		00002	/* other may write */
#define S_IXOTH		00001	/* other may execute */

#define S_IREAD		S_IRUSR	/* owner may read */
#define S_IWRITE	S_IWUSR	/* owner may write */
#define S_IEXEC		S_IXUSR	/* owner may execute */

#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)

/*
 * Function prototypes for POSIX that use the VSTa stat mechanism
 */
extern int fstat(int fd, struct stat *s);
extern int stat(const char *f, struct stat *s);
extern int isatty(int fd);

#endif /* _STAT_H */
@


1.8
log
@Fix typo
@
text
@d32 10
d48 19
a66 3
#define S_IREAD		0x0004	/* owner may read */
#define S_IWRITE 	0x0002	/* owner may write */
#define S_IEXEC		0x0001	/* owner may execute */
@


1.7
log
@Add some more POSIX stuff
@
text
@d21 1
a21 1
	inot_t st_ino;
@


1.6
log
@Add rmdir(), add some const definitions
@
text
@d12 1
d14 5
d20 4
a23 2
	ushort st_dev, st_ino;
	ushort st_mode, st_nlink;
d25 3
a27 2
	ulong st_rdev, st_size;
	ulong st_atime, st_mtime, st_ctime;
d29 1
@


1.5
log
@Delete non-POSIX stat(2) stuff
@
text
@d42 1
a42 1
extern int stat(char *f, struct stat *s);
@


1.4
log
@Add some common macros
@
text
@a38 6
 * Function prototypes for the VSTa stat mechanism
 */
extern char *rstat(port_t fd, char *field);
extern int wstat(port_t fd, char *field);

/*
@


1.3
log
@Add stat prototypes
@
text
@d32 6
@


1.2
log
@Move read/write/exec bits to their familiar UNIX location
@
text
@d5 1
a5 1
 *	The "stat" structure
d7 2
a8 2
 * No such thing exists in VSTa; this structure is mocked up for
 * programs requiring such an interface.  Down with large binary
d31 13
@


1.1
log
@Initial revision
@
text
@d28 3
a30 3
#define S_IREAD		0x0400	/* owner may read */
#define S_IWRITE 	0x0200	/* owner may write */
#define S_IEXEC		0x0100	/* owner may execute */
@
