head	1.16;
access;
symbols
	V1_3_1:1.13
	V1_3:1.13
	V1_2:1.8
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.16
date	95.02.04.05.52.58;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.06.21.20.58.35;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.05.30.21.28.28;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.04.03.21.27.13;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.03.23.21.57.14;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.03.08.20.05.55;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.02.28.22.04.33;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.02.27.02.31.03;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.12.27.22.10.50;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.48.09;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.07.06.16.25.53;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.06.30.19.57.42;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.22.17.38.06;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.22.14.51.48;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.21.46.05;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of cluster allocation and such
@


1.16
log
@Add firewall for bad cluster number
@
text
@/*
 * fat.c
 *	Routines for fiddling storage via the File Allocation Table
 *
 * The FAT is read into memory and kept there through the life of
 * the filesystem server, though it is periodically flushed to disk.
 * All internal tables work in terms of a 16-bit FAT; 12-bit FATs
 * are converted as they are read and later written.
 */
#include "dos.h"
#include <std.h>
#include <sys/param.h>
#include <sys/assert.h>
#include <fcntl.h>
#include <syslog.h>

static ushort *fat,	/* Our in-core FAT */
	*fat12;		/*  ...12-bit version, if needed */
static uint fatlen,	/* Length in FAT16 format */
	fat12len;	/*  ...FAT12, if we're using FAT12 */
static ushort *fatNFAT;
static uchar *dirtymap;	/* Map of sectors which dirty FAT entries */
static uint		/*  ...size of map */
	dirtymapsize;
uint clsize;		/* Size of cluster, in bytes */
static int fat_dirty;	/* Flag that we need to flush the FAT */
static uint nclust;	/* Total clusters on disk */
ulong data0;		/* Byte offset for data block 0 (cluster 2) */
uint dirents;		/* # directory entries */

/*
 * DIRTY()
 *	Mark dirtymap at given position
 */
#define DIRTY(idx) (dirtymap[(idx*sizeof(ushort))/SECSZ] = 1)

/*
 * fat12_fat16()
 *	Convert from FAT12 to FAT16
 */
static void
fat12_fat16(ushort *fat12, ushort *fat16, uint len)
{
	uchar *from, *fatend;
	uint x;
	ushort *u = fat16;

	/*
	 * Scan across, converting 1.5 bytes into 2 bytes
	 */
	from = (uchar *)fat12;
	fatend = from+len;
	for (;;) {
		x = *from++;
		if (from >= fatend) break;
		x = x | ((*from & 0xF) << 8);
		*fat16++ = x;
		x = ((*from++ & 0xF0) >> 4);
		if (from >= fatend) break;
		x = x | (*from++ << 4);
		*fat16++ = x;
		if (from >= fatend) break;
	}

	/*
	 * Now re-scan, converting the "end" marks into 16-bit format
	 */
	for ( ; u < fat16; ++u) {
		if (*u >= 0xFF0) {
			*u |= 0xF000;
		}
	}
}

/*
 * fat16_fat12()
 *	Convert back from FAT16 to FAT12 format
 */
static void
fat16_fat12(ushort *fat16, ushort *fat12, uint len)
{
	ushort *fatend;
	uint x;
	uchar *dest;

	/*
	 * Scan across the FAT16's and assemble them back into
	 * 12-bit format.
	 */
	fatend = (ushort *)(((char *)fat16) + len);
	dest = (uchar *)fat12;
	while (fat16 < fatend) {
		x = *fat16++;
		*dest++ = (x & 0xFF);
		if (fat16 < fatend) {
			*dest++ = ((x & 0xF00) >> 8) |
				((*fat16 & 0xF) << 4);
		} else {
			*dest++ = ((x & 0xF00) >> 8);
			break;
		}
		*dest++ = ((*fat16++ & 0xFF0) >> 4);
	}
}

/*
 * fat_init()
 *	Initialize FAT handling
 *
 * Read in the FAT, convert if needed.
 */
void
fat_init(void)
{
	uint x;

	/*
	 * Calculate some static values
	 */
	ASSERT((bootb.secsize0 + (bootb.secsize1 << 8)) == SECSZ,
		"fat_init: bad sector size");
	dirents = bootb.dirents0 + (bootb.dirents1 << 8);
	clsize = bootb.clsize * SECSZ;
	x = bootb.psect0 + (bootb.psect1 << 8);
	if (x > 0) {
		nclust = (x * SECSZ) / clsize;
	} else {
		nclust = (bootb.bigsect * SECSZ) / clsize;
	}
	data0 = bootb.nrsvsect + (bootb.nfat * bootb.fatlen) +
		(dirents * sizeof(struct directory))/SECSZ;
	data0 *= SECSZ;

	/*
	 * Get map of dirty sectors in FAT
	 */
	dirtymapsize = roundup(nclust*sizeof(ushort), SECSZ) / SECSZ;
	dirtymap = malloc(dirtymapsize);
	ASSERT(dirtymap, "fat_init: dirtymap");
	bzero(dirtymap, dirtymapsize);
	fat_dirty = 0;

	/*
	 * Convert FAT-12 to FAT-16
	 */
	if (nclust < 4086) {
		fat12 = malloc(bootb.fatlen * SECSZ);
		if (fat12 == 0) {
			perror("fat_init: fat12");
			exit(1);
		}

		/*
		 * The length is 1/3 greater than the FAT-12's size.
		 * We add "3" for slop having to do with the integer
		 * division.
		 */
		fat12len = bootb.fatlen * SECSZ;
		fatlen = fat12len + (fat12len/3) + 3;
	} else {
		/*
		 * FAT16--no conversion needed
		 */
		fat12 = 0;
		fatlen = bootb.fatlen * SECSZ;
	}

	/*
	 * Get memory for FAT table
	 */
	fat = malloc(fatlen);
	if (fat == 0) {
		perror("fat_init");
		exit(1);
	}
	fatNFAT = fat + nclust;

	/*
	 * Seek to FAT table on disk, read into buffer
	 */
	lseek(blkdev, 1 * SECSZ, 0);
	if (fat12) {
		if (read(blkdev, fat12, fat12len) != fat12len) {
			syslog(LOG_ERR, "read (%d bytes) of FAT12 failed",
				fat12len);
			exit(1);
		}
		fat12_fat16(fat12, fat, fat12len);
	} else {
		if (read(blkdev, fat, fatlen) != fatlen) {
			syslog(LOG_ERR, "read (%d bytes) of FAT failed",
				fatlen);
			exit(1);
		}
	}
}

/*
 * clust_setlen()
 *	Twiddle FAT allocation to match the indicated length
 *
 * Returns 0 if it could be done; 1 if it failed.
 */
int
clust_setlen(struct clust *c, ulong newlen)
{
	uint newclust, x, y;
	ushort *ctmp;

	/*
	 * Figure out how many clusters are needed now
	 */
	newclust = roundup(newlen, clsize) / clsize;

	/*
	 * If no change in allocation, just return success
	 */
	if (c->c_nclust == newclust) {
		return(0);
	}

	/*
	 * Getting smaller--free stuff at the end
	 */
	if (c->c_nclust > newclust) {
		for (x = newclust; x < c->c_nclust; ++x) {
			y = c->c_clust[x];
#ifdef DEBUG
			if ((y >= nclust) || (y < 2)) {
				uint z;

				syslog(LOG_ERR, "bad cluster 0x%x", y);
				syslog(LOG_ERR, "clusters in file:");
				for (z = 0; z < c->c_nclust; ++z) {
					syslog(LOG_ERR, " %x", c->c_clust[z]);
				}
				ASSERT(0, "fat_setlen: bad clust");
			}
#endif
			fat[y] = 0;
			DIRTY(y);
		}
		if (newclust > 0) {
			fat[y = c->c_clust[newclust-1]] = FAT_EOF;
			DIRTY(y);
		}
		c->c_nclust = newclust;
		fat_dirty = 1;
		return(0);
	}

	/*
	 * Trying to grow.  See if we can get the blocks.  If we can't,
	 * we bail out.  The allocation is done in two passes, so that
	 * when we bail after running out of space there's nothing which
	 * needs to be undone.  The realloc()'ed c_clust is harmless.
	 */
	ctmp = realloc(c->c_clust, newclust * sizeof(ushort));
	if (ctmp == 0) {
		return(1);
	}
	c->c_clust = ctmp;
	y = 0;
	for (x = c->c_nclust; x < newclust; ++x) {
		/*
		 * Scan for next free cluster
		 */
		while ((y < nclust) && fat[y]) {
			y += 1;
		}

		/*
		 * If we didn't find one, roll back and fail.
		 */
		if (y >= nclust) {
			return(1);
		}

		/*
		 * Sanity
		 */
		ASSERT_DEBUG(y >= 2, "clust_setlen: bad FAT");

		/*
		 * Otherwise add it to our array.  We will flag it
		 * consumed soon.
		 */
		ctmp[x] = y++;
	}

	/*
	 * When we get here, the new clusters for the file extension
	 * have been found and filled into the c_clust[] array.
	 * We now go back and (1) flag the FAT entries consumed,
	 * which also builds the cluster chain, and (2) update the
	 * count of clusters for this file.
	 */

	/*
	 * Chain last block which already existed onto this new
	 * space.
	 */
	if (c->c_nclust > 0) {
		fat[y = c->c_clust[c->c_nclust-1]] = c->c_clust[c->c_nclust];
		DIRTY(y);
	}

	/*
	 * Chain all the new clusters together
	 */
	for (x = c->c_nclust; x < newclust-1; ++x) {
		fat[y = c->c_clust[x]] = c->c_clust[x+1];
		DIRTY(y);
	}

	/*
	 * Mark the EOF cluster for the last one
	 */
	fat[y = c->c_clust[newclust-1]] = FAT_EOF;
	DIRTY(y);
	c->c_nclust = newclust;
	fat_dirty = 1;
	return(0);
}

/*
 * alloc_clust()
 *	Allocate a description of the given cluster chain
 */
struct clust *
alloc_clust(uint start)
{
	uint nclust = 1;
	uint x;
	struct clust *c;

	/*
	 * Get the cluster description
	 */
	c = malloc(sizeof(struct clust));
	if (c == 0) {
		return(0);
	}

	/*
	 * Zero-length file is easy
	 */
	if (start == 0) {
		c->c_nclust = 0;
		c->c_clust = 0;
		return(c);
	}

	/*
	 * Scan the chain to get its length
	 */
	for (x = start; fat[x] < FAT_RESERVED; x = fat[x]) {
		ASSERT_DEBUG(x >= 2, "alloc_clust: free cluster in file");
		nclust++;
	}

	/*
	 * Allocate the description array
	 */
	c->c_clust = malloc(nclust * sizeof(ushort));
	if (c->c_clust == 0) {
		return(0);
	}
	c->c_nclust = nclust;

	/*
	 * Scan again, recording each cluster number
	 */
	x = start;
	nclust = 0;
	do {
		c->c_clust[nclust++] = x;
		x = fat[x];
	} while (x < FAT_RESERVED);
	return(c);
}

/*
 * free_clust()
 *	Free a cluster description
 */
void
free_clust(struct clust *c)
{
	if (c->c_clust) {
		free(c->c_clust);
	}
	free(c);
}

/*
 * map_write()
 *	Write a FAT16 using the dirtymap to minimize I/O
 */
static void
map_write(void)
{
	int x, cnt, pass;
	off_t off;

	/*
	 * There are two copies of the FAT, so do them iteratively
	 */
	for (pass = 0; pass <= 1; ++pass) {
		/*
		 * Calculate the offset once per pass
		 */
		off = pass*(long)fatlen;

		/*
		 * Walk across the dirty map, find the next dirty sector
		 * of FAT information to write out.
		 */
		for (x = 0; x < dirtymapsize; ) {
			/*
			 * Not dirty.  Advance to next sector's worth.
			 */
			if (!dirtymap[x]) {
				x += 1;
				continue;
			}

			/*
			 * Now find runs, so we can flush adjacent sectors
			 * in a single operation.
			 */
			for (cnt = 1; ((x+cnt) < dirtymapsize) &&
					dirtymap[x+cnt]; ++cnt) {
				;
			}

			/*
			 * Seek to the right place, and write the data
			 */
			lseek(blkdev, SECSZ + x*SECSZ + off, 0);
			if (write(blkdev,
					&fat[x*SECSZ/sizeof(ushort)],
					SECSZ*cnt) != (SECSZ*cnt)) {
				perror("fat16");
				syslog(LOG_ERR, "write of FAT16 #%d failed",
					pass);
				exit(1);
			}

			/*
			 * Advance x
			 */
			x += cnt;
		}
	}

	/*
	 * Clear dirtymap--everything's been flushed successfully
	 */
	bzero(dirtymap, dirtymapsize);
	fat_dirty = 0;
}

/*
 * fat_sync()
 *	Sync out the FAT to disk
 *
 * Both copies are updated; if the first copy can not be written
 * successfully, the second is left alone and the server aborts.
 */
void
fat_sync(void)
{
	/*
	 * Not dirty--no work
	 */
	if (!fat_dirty) {
		return;
	}

	/*
	 * Seek to start of FATs, write them out
	 */
	if (fat12) {
		int x;

		/*
		 * FAT12--write whole thing.  FAT12 entries can span
		 * sectors, so it needs its own loop. XXX
		 */
		lseek(blkdev, 1 * SECSZ, 0);
		fat16_fat12(fat, fat12, fatlen);
		x = write(blkdev, fat12, fat12len);
		if (x!= fat12len) {
			perror("fat12");
			syslog(LOG_ERR, "write of FAT12 #1 failed, ret %d", x);
			exit(1);
		}
		if (write(blkdev, fat12, fat12len) != fat12len) {
			perror("fat12");
			syslog(LOG_ERR, "write of FAT12 #2 failed");
			exit(1);
		}
		fat_dirty = 0;
	} else {
		map_write();
	}
}

/*
 * get_clust0()
 *	Get first cluster # of first cluster
 *
 * Used to fill in the "start" field of dir entries like ".."
 */
uint
get_clust0(struct clust *c)
{
	ASSERT_DEBUG(c->c_nclust > 0, "get_clust0: no data");
	return(c->c_clust[0]);
}
@


1.15
log
@Convert to openlog()
@
text
@d280 5
@


1.14
log
@Syslog and time support
@
text
@a30 3
extern char		/* String used as a prefix in syslog calls */
	dos_sysmsg[];
	
d184 2
a185 2
			syslog(LOG_ERR, "%s read (%d bytes) of FAT12 failed",
				dos_sysmsg, fat12len);
d191 2
a192 2
			syslog(LOG_ERR, "%s read (%d bytes) of FAT failed",
				dos_sysmsg, fatlen);
d232 2
a233 4
				syslog(LOG_ERR, "%s bad cluster 0x%x",
					dos_sysmsg, y);
				syslog(LOG_ERR, "%s clusters in file:",
					dos_sysmsg);
d235 1
a235 2
					syslog(LOG_ERR, "%s  %x",
						dos_sysmsg, c->c_clust[z]);
d440 2
a441 2
				syslog(LOG_ERR, "%s write of FAT16 #%d failed",
					dos_sysmsg, pass);
d491 1
a491 3
			syslog(LOG_ERR,
				"%s write of FAT12 #1 failed, ret %d",
				dos_sysmsg, x);
d496 1
a496 2
			syslog(LOG_ERR,
				"%s write of FAT12 #2 failed", dos_sysmsg);
@


1.13
log
@Add renaming/moving of directories
@
text
@d31 3
d187 2
a188 2
			syslog(LOG_ERR, "Read (%d bytes) of FAT12 failed\n",
				fat12len);
d194 2
a195 2
			syslog(LOG_ERR, "Read (%d bytes) of FAT failed\n",
				fatlen);
d235 4
a238 2
				syslog(LOG_ERR, "Bad cluster 0x%x\n", y);
				syslog(LOG_ERR, "Clusters in file:\n");
d240 2
a241 1
					syslog(LOG_ERR, " %x", c->c_clust[z]);
d446 2
a447 2
				syslog(LOG_ERR, "Write of FAT16 #%d failed\n",
					pass);
d498 2
a499 1
				"Write of FAT12 #1 failed, ret %d\n", x);
d505 1
a505 1
				"Write of FAT12 #2 failed\n");
@


1.12
log
@Fix -Wall warnings
@
text
@d506 13
@


1.11
log
@Fix up handling of dir entry timestamps.  Clear fat_dirty
flag in a couple places.  Avoid a needless lseek() for the
common case.
@
text
@d14 1
a16 3
extern int blkdev;	/* Our disk */
extern struct boot	/* Boot block */
	bootb;
d204 1
@


1.10
log
@Convert to syslog()
@
text
@d143 1
d457 1
a479 1
	lseek(blkdev, 1 * SECSZ, 0);
d487 1
d502 1
@


1.9
log
@Fix serious miscalculation of FAT size; left higher half
of FAT un-sync'ed.
@
text
@d14 1
d185 1
a185 1
			printf("Read (%d bytes) of FAT12 failed\n",
d192 1
a192 1
			printf("Read (%d bytes) of FAT failed\n",
d232 2
a233 2
				printf("Bad cluster 0x%x\n", y);
				printf("Clusters in file:\n");
d235 1
a235 1
					printf(" %x", c->c_clust[z]);
d440 1
a440 1
				printf("Write of FAT16 #%d failed\n",
d490 2
a491 1
			printf("Write of FAT12 #1 failed, ret %d\n", x);
d496 2
a497 1
			printf("Write of FAT12 #2 failed\n");
@


1.8
log
@Improve error message for FAT read
@
text
@d138 1
a138 1
	dirtymapsize = roundup(nclust, SECSZ) / SECSZ;
@


1.7
log
@Source reorg
@
text
@d184 2
a185 1
			printf("Read of FAT12 failed\n");
d191 2
a192 1
			printf("Read of FAT failed\n");
@


1.6
log
@Only write out portions of FAT which have been changed.  Reduces
disk I/O substantially.
@
text
@d10 1
a10 1
#include <dos/dos.h>
@


1.5
log
@Fix struct alignment, can't assume bitfields will fall on byte
boundary any more.
@
text
@d23 3
d33 6
d136 8
d238 1
d241 2
a242 1
			fat[c->c_clust[newclust-1]] = FAT_EOF;
d296 2
a297 1
		fat[c->c_clust[c->c_nclust-1]] = c->c_clust[c->c_nclust];
d304 2
a305 1
		fat[c->c_clust[x]] = c->c_clust[x+1];
d311 2
a312 1
	fat[c->c_clust[newclust-1]] = FAT_EOF;
d389 67
d479 4
d496 1
a496 8
		if (write(blkdev, fat, fatlen) != fatlen) {
			printf("Write of FAT #1 failed\n");
			exit(1);
		}
		if (write(blkdev, fat, fatlen) != fatlen) {
			printf("Write of FAT #2 failed\n");
			exit(1);
		}
@


1.4
log
@Fix pointer arithmetic error.  Also add useful assert
to catch garbage in the FAT chains.
@
text
@d27 1
d107 2
d112 3
a114 1
	ASSERT(bootb.secsize == SECSZ, "fat_init: bad sector size");
d116 3
a118 2
	if (bootb.psect > 0) {
		nclust = (bootb.psect * SECSZ) / clsize;
d123 1
a123 1
		(bootb.dirents * sizeof(struct directory))/SECSZ;
@


1.3
log
@When truncating, make sure to flag the new "last" cluster with
an EOF mark in the FAT.
@
text
@d81 1
a81 1
	fatend = fat16+len;
d322 1
@


1.2
log
@Add flush of FAT
@
text
@d216 4
@


1.1
log
@Initial revision
@
text
@d12 1
d187 1
a187 1
	newclust = newlen / clsize;
d353 48
@
