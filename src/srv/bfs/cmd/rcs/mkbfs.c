head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.4
date	94.04.10.19.54.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.03.07.18.08.44;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.02.02.19.57.40;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.39.49;	author vandys;	state Exp;
branches;
next	;


desc
@Filesystem creation
@


1.4
log
@Support "fast" mkfs'ing, not writing all blocks
@
text
@/*
 * Filename:	mkbfs.c
 * Originated:	Andy Valencia
 * Updated By:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Last Update: 8th April 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description: Create a bfs filesystem
 *
 * We assume we can just fopen(..., "w") the named file, and make ourselves
 * a filesystem.  We writes all of the blocks if a fs size is specified on the
 * command line, which is probably not desirable for a "mkfs" util.  This does
 * however allow a bfs to be built on top of an existing fs - very useful for
 * avoiding repartitioning, or for running bfs where no fs partitioning
 * mechanism exists (PC FDDs).  If no blocks parameter is spec'd we try and
 * determine the fs size automagically!
 */


#include <fdl.h>
#include <stat.h>
#include <std.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../bfs.h"


void
main(int argc, char **argv)
{
	FILE *fp;
	int fd;
	int write_all_blocks = 0;
	struct super sb;
	struct dirent d;
	uchar block[BLOCKSIZE];
	int x, nblocks, zblocks, dblocks = 8;
	char *statstr;

	if (argc != 3 && argc != 2) {
		fprintf(stderr,
			"Usage: %s <device_or_fs_file> [number_of_blocks]\n",
			argv[0]);
		exit(1);
	}

	if ((fp = fopen(argv[1], "wb")) == NULL) {
		perror(argv[1]);
		exit(1);
	}

	/*
	 * How many fs blocks are we going for?
	 */
	if (argc == 2) {
		/*
		 * Try to work out the number of fs blocks via a stat call
		 */
		fd = fileno(fp);
		statstr = rstat(__fd_port(fd), "size");
		if (statstr == NULL ) {
			fprintf(stderr,
				"Unable to stat size of: %s\n", argv[1]);
			exit(1);
		}
		sscanf(statstr, "%d", &nblocks);
		nblocks /= BLOCKSIZE;
	} else {
		/*
		 * Get the number of fs blocks off the command line
		 */
		if (sscanf(argv[2], "%d", &nblocks) != 1) {
			fprintf(stderr,
				"Illegal number of blocks: %s\n", argv[2]);
			exit(1);
		}
		write_all_blocks = 1;
	}

	printf("Creating a 'bfs' fs on %s of %d blocks\n", argv[1], nblocks);

	/*
	 * Create superblock details and run sanity checks
	 */
	sb.s_magic = SMAGIC;
	sb.s_blocks = nblocks;
	sb.s_supstart = 0;
	sb.s_supblocks = BLOCKS(sizeof(struct super));
	sb.s_dirstart = sb.s_supstart + sb.s_supblocks;
	sb.s_dirblocks = dblocks;
	sb.s_datastart = sb.s_dirblocks + sb.s_dirblocks;
	sb.s_datablocks = sb.s_free = nblocks - dblocks - sb.s_supblocks;
	sb.s_nextfree = dblocks + sb.s_dirstart;
	sb.s_ndirents = dblocks * (BLOCKSIZE / sizeof(struct dirent));
	sb.s_direntsize = sizeof(struct dirent);

	if (sb.s_datablocks < MINDATABLOCKS) {
		fprintf(stderr,
			"Too few data blocks (%d) with %d fs blocks\n",
			sb.s_datablocks, nblocks);
		exit(1);
	}

	/*
	 * Zero out the superblock, dir space and possibly all of the fs space
	 */
	memset(block, '\0', BLOCKSIZE);
	zblocks = write_all_blocks ? nblocks : sb.s_supblocks + dblocks;
	for (x = 0; x < zblocks; ++x) {
		fseek(fp, x * BLOCKSIZE, SEEK_SET);
		if (fwrite(block, sizeof(char), BLOCKSIZE, fp)
		    		!= BLOCKSIZE) {
			perror(argv[1]);
			exit(1);
		}
	}
	fseek(fp, 0, SEEK_SET);

	/*
	 * Write the superblock
	 */
	memcpy(block, &sb, sizeof(struct super));
	if (fwrite(block, BLOCKSIZE, 1, fp) != 1) {
		perror(argv[1]);
		exit(1);
	}
	fflush(fp);

	/*
	 * Write directory entries until we can't fit any more in
	 * sb.s_dirblocks.  No directory entry must cross a block boundary,
	 * so we skip to beginning of next block in case we hit an alignment
	 * problem.
	 */
	x = 0;
	memset(d.d_name, '\0', BFSNAMELEN);
	d.d_start = d.d_len = 0;
	fseek(fp, sb.s_dirstart * BLOCKSIZE, SEEK_SET);
	while ((ftell(fp) + sizeof(d)) <= ((dblocks + 1) * BLOCKSIZE)) {
		int curblk, endblk;

		curblk = ftell(fp) / BLOCKSIZE;
		endblk = (ftell(fp) + sizeof(d) - 1) / BLOCKSIZE;
		if (curblk != endblk) {
			fseek(fp, endblk * BLOCKSIZE, SEEK_SET);
		}
		d.d_inum = x++;
		if (fwrite(&d, sizeof(d), 1, fp) != 1) {
			perror(argv[1]);
			exit(1);
		}
	}

	fclose(fp);
}
@


1.3
log
@Create a BFS filesystem
@
text
@d5 1
a5 1
 * Last Update: 11th February 1994
d8 1
a8 1
 * Description: Create a BFS filesystem
d10 7
a16 5
 * This guy assumes he can just fopen(..., "w") the named file, and make
 * himself a filesystem.  He writes all the blocks, which is probably not
 * desirable for a native mkfs.  This does however allow a bfs to be built
 * on top of an existing fs - very useful for avoiding repartitioning, or
 * for running bfs where no fs partitioning mechanism exists (PC FDDs).
d20 2
d33 2
d38 2
a39 1
	int x, nblocks, dblocks = 4;
d41 4
a44 2
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <device> <blocks>\n", argv[0]);
d47 1
a47 4
	if (sscanf(argv[2], "%d", &nblocks) != 1) {
		fprintf(stderr, "Illegal number: %s\n", argv[2]);
		exit(1);
	}
d54 1
a54 1
	 * Zero out the whole thing first.
d56 20
a75 5
	memset(block, '\0', BLOCKSIZE);
	for (x = 0; x < nblocks; ++x) {
		if (fwrite(block, sizeof(char), BLOCKSIZE, fp)
		    		!= BLOCKSIZE) {
			perror(argv[1]);
d78 1
d80 2
a81 1
	fseek(fp, 0, SEEK_SET);
d84 1
a84 1
	 * Create a superblock and write it out
d97 28
a124 1
	if (fwrite(&sb, sizeof(sb), 1, fp) != 1) {
d128 1
a148 1
printf("%d %d!  ", d.d_inum, ftell(fp));
d154 2
@


1.2
log
@Convert to native
@
text
@d2 5
a6 2
 * mkfs.c
 *	Create a BFS filesystem
d8 7
a14 6
 * This guy assumes he can just fopen(..., "w") the named file, and
 * make himself a filesystem.  He writes all the blocks, which is
 * probably not desirable for a native mkfs.  However, under DOS,
 * this behavior allows us to test with a DOS file playing the part
 * of a block device.  DOS doesn't get sparse files right, in my
 * experience.
d16 3
d20 2
d24 3
a26 3
main(argc, argv)
	int argc;
	char **argv;
d31 2
a32 2
	char block[BLOCKSIZE];
	int x, nblocks;
d48 1
a48 1
	 * Zero out the whole thing first
d52 2
a53 2
		if (fwrite(block, sizeof(char), BLOCKSIZE, fp) !=
				BLOCKSIZE) {
d58 1
a58 1
	rewind(fp);
d61 1
a61 2
	 * Create a superblock, write it out.  Note that the dummy
	 * directory entry on the end is not included.
d64 11
a74 4
	sb.s_nblocks = nblocks;
	sb.s_free = nblocks-NDIRBLOCKS;
	sb.s_nextfree = NDIRBLOCKS;
	if (fwrite(&sb, sizeof(sb)-sizeof(struct dirent), 1, fp) != 1) {
d81 3
a83 2
	 * NDIRBLOCKS.  No directory entry crosses a block boundary,
	 * so skip to beginning of next block in these cases.
d86 1
a86 1
	memset(d.d_name, '\0', NAMELEN);
d88 8
a95 7
	while ((ftell(fp)+sizeof(d)) < (NDIRBLOCKS*BLOCKSIZE)) {
		int curblk, nextblk;

		curblk = ftell(fp)/BLOCKSIZE;
		nextblk = (ftell(fp)+sizeof(d))/BLOCKSIZE;
		if (curblk != nextblk) {
			fseek(fp, nextblk*BLOCKSIZE, 0);
d98 1
@


1.1
log
@Initial revision
@
text
@d13 1
a13 1
#include <bfs/bfs.h>
@
