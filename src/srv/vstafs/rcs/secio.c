head	1.8;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5
	V1_1:1.5;
locks; strict;
comment	@ * @;


1.8
date	94.06.21.20.58.58;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.05.30.21.28.15;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.02.28.22.06.05;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.09.18.17.34.01;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.29.22.26.48;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.29.19.18.05;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.09.00.09;	author vandys;	state Exp;
branches;
next	;


desc
@Basic interface for sector-based I/O
@


1.8
log
@Convert to openlog()
@
text
@/*
 * secio.c
 *	Routines for reading and writing sectors
 *
 * The problem of I/O errors is handled using a simplistic approach.  All
 * of the filesystem is designed so that on-disk structures will be
 * consistent if the filesystem is aborted at any given point in time.
 * Rather than further complicate this, we simply abort on I/O error,
 * and fall back onto this design.
 *
 * Lots of filesystems have strategies for going on in the face of I/O
 * errors.  Lots of filesystems get scary after an I/O error.  Use a
 * mirror if you really care.
 */
#include "vstafs.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/assert.h>
#include <stdio.h>
#include <std.h>
#include <syslog.h>

extern int blkdev;

/*
 * do_read()/do_write()/do_seek()
 *	Wrappers so we can share debugging complaints
 */
static void
do_read(void *buf, uint nbyte)
{
	int x;

	x = read(blkdev, buf, nbyte);
#ifdef DEBUG
	if (x != nbyte) {
		perror("read");
		syslog(LOG_ERR, "read(%d, %x, %d) returns %d",
			blkdev, (uint)buf, nbyte, x);
	}
#endif
	ASSERT(x == nbyte, "do_read: I/O failed");
}
static void
do_write(void *buf, uint nbyte)
{
	int x;

	x = write(blkdev, buf, nbyte);
#ifdef DEBUG
	if (x != nbyte) {
		perror("write");
		syslog(LOG_ERR, "write(%d, %x, %d) returns %d\n",
			blkdev, (uint)buf, nbyte, x);
	}
#endif
	ASSERT(x == nbyte, "do_write: I/O failed");
}
static void
do_lseek(off_t off)
{
	off_t o;

	o = lseek(blkdev, off, SEEK_SET);
#ifdef DEBUG
	if (o != off) {
		perror("lseek");
		syslog(LOG_ERR, "lseek(%d, %ld) returns %ld\n",
			blkdev, off, o);
	}
#endif
	ASSERT(o == off, "do_lseek: seek failed");
}

/*
 * read_sec()
 *	Read a sector
 */
void
read_sec(daddr_t d, void *p)
{
	do_lseek(stob(d));
	do_read(p, SECSZ);
}

/*
 * write_sec()
 *	Write a sector
 */
void
write_sec(daddr_t d, void *p)
{
	do_lseek(stob(d));
	do_write(p, SECSZ);
}

/*
 * read_secs()
 *	Read sectors
 */
void
read_secs(daddr_t d, void *p, uint nsec)
{
	do_lseek(stob(d));
	do_read(p, stob(nsec));
}

/*
 * write_secs()
 *	Write sectors
 */
void
write_secs(daddr_t d, void *p, uint nsec)
{
	do_lseek(stob(d));
	do_write(p, stob(nsec));
}
@


1.7
log
@Syslog support
@
text
@d38 2
a39 2
		syslog(LOG_ERR, "%s read(%d, %x, %d) returns %d",
			vfs_sysmsg, blkdev, (uint)buf, nbyte, x);
d53 2
a54 2
		syslog(LOG_ERR, "%s write(%d, %x, %d) returns %d\n",
			vfs_sysmsg, blkdev, (uint)buf, nbyte, x);
d68 2
a69 2
		syslog(LOG_ERR, "%s lseek(%d, %ld) returns %ld\n",
			vfs_sysmsg, blkdev, off, o);
@


1.6
log
@Convert to syslog()
@
text
@d38 2
a39 2
		syslog(LOG_ERR, "read(%d, %x, %d) returns %d\n",
			blkdev, (uint)buf, nbyte, x);
d53 2
a54 2
		syslog(LOG_ERR, "write(%d, %x, %d) returns %d\n",
			blkdev, (uint)buf, nbyte, x);
d68 2
a69 2
		syslog(LOG_ERR, "lseek(%d, %ld) returns %ld\n",
			blkdev, off, o);
@


1.5
log
@Source reorg
@
text
@d21 1
d38 1
a38 1
		fprintf(stderr, "read(%d, %x, %d) returns %d\n",
d53 1
a53 1
		fprintf(stderr, "write(%d, %x, %d) returns %d\n",
d68 1
a68 1
		fprintf(stderr, "lseek(%d, %ld) returns %ld\n",
@


1.4
log
@Improve diagnostics, share code among all interfaces
@
text
@d15 1
a15 1
#include <vstafs/vstafs.h>
@


1.3
log
@Clean up -Wall warnings
@
text
@d19 2
d25 50
d81 2
a82 5
	off_t o;

	o = (off_t)stob(d);
	ASSERT(lseek(blkdev, o, SEEK_SET) == o, "read_sec: seek error");
	ASSERT(read(blkdev, p, SECSZ) == SECSZ, "read_sec: I/O error");
d92 2
a93 5
	off_t o;

	o = (off_t)stob(d);
	ASSERT(lseek(blkdev, o, SEEK_SET) == o, "write_sec: seek error")
	ASSERT(write(blkdev, p, SECSZ) == SECSZ, "write_sec: I/O error");
d103 2
a104 6
	off_t o;
	uint sz = stob(nsec);

	o = (off_t)stob(d);
	ASSERT(lseek(blkdev, o, SEEK_SET) == o, "read_sec: seek error");
	ASSERT(read(blkdev, p, sz) == sz, "read_secs: I/O error");
d114 2
a115 6
	off_t o;
	uint sz = stob(nsec);

	o = (off_t)stob(d);
	ASSERT(lseek(blkdev, o, SEEK_SET) == o, "write_sec: seek error");
	ASSERT(write(blkdev, p, sz) == sz, "write_secs: I/O error");
@


1.2
log
@Convert to ASSERT()
@
text
@d17 1
@


1.1
log
@Initial revision
@
text
@d15 1
a15 1
#include <stdio.h>
d17 1
a17 1
#include <vstafs/vstafs.h>
d31 2
a32 9
	if (lseek(blkdev, o, SEEK_SET) != o) {
		fprintf(stderr, "read_sec: seek error to off %ld\n", (long)o);
		exit(1);
	}
	if (read(blkdev, p, SECSZ) != SECSZ) {
		fprintf(stderr, "read_sec: read error off %ld buf 0x%x\n",
			(long)o, p);
		exit(1);
	}
d45 2
a46 9
	if (lseek(blkdev, o, SEEK_SET) != o) {
		fprintf(stderr, "write_sec: seek error to off %ld\n", (long)o);
		exit(1);
	}
	if (write(blkdev, p, SECSZ) != SECSZ) {
		fprintf(stderr, "write_sec: write error off %ld buf 0x%x\n",
			(long)o, p);
		exit(1);
	}
d60 2
a61 10
	if (lseek(blkdev, o, SEEK_SET) != o) {
		fprintf(stderr, "read_sec: seek error to off %ld\n", (long)o);
		exit(1);
	}
	if (read(blkdev, p, sz) != sz) {
		fprintf(stderr,
			"read_secs: read error off %ld buf 0x%x size %d\n",
			(long)o, p, sz);
		exit(1);
	}
d75 2
a76 10
	if (lseek(blkdev, o, SEEK_SET) != o) {
		fprintf(stderr, "write_sec: seek error to off %ld\n", (long)o);
		exit(1);
	}
	if (write(blkdev, p, sz) != sz) {
		fprintf(stderr,
			"write_secs: write error off %ld buf 0x%x size %d\n",
			(long)o, p, sz);
		exit(1);
	}
@
