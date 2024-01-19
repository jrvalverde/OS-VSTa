head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4;
locks; strict;
comment	@ * @;


1.5
date	94.05.24.16.57.36;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.10.03.20.37.54;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.10.03.19.03.46;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.09.27.23.10.50;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.09.27.18.57.24;	author vandys;	state Exp;
branches;
next	;


desc
@Make filesystem
@


1.5
log
@Fix -p prealloc switch
@
text
@/*
 * mkfs.c
 *	Write an initial filesystem image onto the named file
 */
#include <sys/types.h>
#include "../vstafs.h"
#include <stdio.h>
#include <std.h>

static FILE *fp;	/* Device we're filling in */
void *secbuf;		/* A utility sector buffer */

/*
 * write_header()
 *	Write the initial sector
 */
static void
write_header(ulong nsec)
{
	struct fs *f = secbuf;

	fseek(fp, (off_t)BASE_SEC*SECSZ, 0);
	f->fs_magic = FS_MAGIC;
	f->fs_size = nsec;
	f->fs_extsize = EXTSIZ;
	f->fs_free = FREE_SEC;
	fwrite(f, SECSZ, 1, fp);
}

/*
 * write_root()
 *	Write the root directory--initially, no entries
 */
static void
write_root(void)
{
	struct fs_file *fs = secbuf;

	fseek(fp, (off_t)ROOT_SEC*SECSZ, 0);
	fs->fs_prev = 0;
	fs->fs_rev = 1;
	fs->fs_len = sizeof(struct fs_file);
	fs->fs_type = FT_DIR;
	fs->fs_nlink = 1;
	fs->fs_prot.prot_len = 2;
	fs->fs_prot.prot_default = ACC_READ;
	fs->fs_prot.prot_bits[0] = 0;
	fs->fs_prot.prot_id[0] = 1;
	fs->fs_prot.prot_bits[1] = ACC_WRITE|ACC_CHMOD;
	fs->fs_prot.prot_id[1] = 1;
	fs->fs_owner = 0;
	fs->fs_nblk = 1;
	fs->fs_blks[0].a_start = ROOT_SEC;
	fs->fs_blks[0].a_len = 1;
	fwrite(fs, SECSZ, 1, fp);
}

/*
 * write_freelist()
 *	Put rest of blocks into initial freelist
 */
static void
write_freelist(ulong nsec)
{
	struct free *f = secbuf;

	fseek(fp, (off_t)FREE_SEC*SECSZ, 0);
	bzero(f, SECSZ);
	f->f_next = 0;
	f->f_nfree = 1;
	f->f_free[0].a_start = FREE_SEC+1;
	f->f_free[0].a_len = nsec-(FREE_SEC+1);
	fwrite(f, SECSZ, 1, fp);
}

/*
 * usage()
 *	Complain & bail
 */
static void
usage(void)
{
	printf("Usage is: mkfs [-p] <device> <nsector>\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	ulong x, nsec;
	int prealloc = 0;

	/*
	 * Check/parse args
	 */
	if (argc < 3) {
		usage();
	}
	if (argc > 3) {
		if (strcmp(argv[1], "-p")) {
			usage();
		}
		prealloc = 1;
		argv += 1;
	}
	if (sscanf(argv[2], "%ld", &nsec) != 1) {
		printf("Bad <nsector>: %s\n", argv[2]);
		exit(1);
	}
	fp = fopen(argv[1], "wb");
	if (fp == 0) {
		perror(argv[1]);
		exit(1);
	}

	/*
	 * Preallocate sectors
	 */
	secbuf = malloc(SECSZ);
	if (prealloc) {
		bzero(secbuf, SECSZ);
		printf("Pre-allocate: "); fflush(stdout);
		for (x = 0; x < nsec; ++x) {
			fwrite(secbuf, SECSZ, 1, fp);
			/* Put a marker each 8K */
			if ((x % ((8*1024)/SECSZ)) == 0) {
				static int col = 0;

				if (++col > 64) {
					col = 0;
					putchar('\n');
				}
				printf("."); fflush(stdout);
			}
		}
	}

	/*
	 * Write data structures
	 */
	write_header(nsec);
	write_root();
	write_freelist(nsec);
	fclose(fp);
	return(0);
}
@


1.4
log
@Always fill device using sector I/O
@
text
@d100 1
a100 1
		if (!strcmp(argv[1], "-p")) {
d104 1
@


1.3
log
@Make prealloc scan optional; not needed for disk
@
text
@d8 1
d11 1
d20 1
a20 1
	struct fs f;
d23 5
a27 5
	f.fs_magic = FS_MAGIC;
	f.fs_size = nsec;
	f.fs_extsize = EXTSIZ;
	f.fs_free = FREE_SEC;
	fwrite(&f, sizeof(struct fs), 1, fp);
d37 1
a37 1
	struct fs_file fs;
d40 16
a55 16
	fs.fs_prev = 0;
	fs.fs_rev = 1;
	fs.fs_len = sizeof(struct fs_file);
	fs.fs_type = FT_DIR;
	fs.fs_nlink = 1;
	fs.fs_prot.prot_len = 2;
	fs.fs_prot.prot_default = ACC_READ;
	fs.fs_prot.prot_bits[0] = 0;
	fs.fs_prot.prot_id[0] = 1;
	fs.fs_prot.prot_bits[1] = ACC_WRITE|ACC_CHMOD;
	fs.fs_prot.prot_id[1] = 1;
	fs.fs_owner = 0;
	fs.fs_nblk = 1;
	fs.fs_blks[0].a_start = ROOT_SEC;
	fs.fs_blks[0].a_len = 1;
	fwrite(&fs, sizeof(struct fs_file), 1, fp);
d65 1
a65 1
	struct free f;
d68 6
a73 6
	bzero(&f, sizeof(f));
	f.f_next = 0;
	f.f_nfree = 1;
	f.f_free[0].a_start = FREE_SEC+1;
	f.f_free[0].a_len = nsec-(FREE_SEC+1);
	fwrite(&f, sizeof(struct free), 1, fp);
a90 1
	char sec[SECSZ];
d118 1
d120 1
a120 1
		bzero(sec, SECSZ);
d123 1
a123 1
			fwrite(sec, SECSZ, 1, fp);
@


1.2
log
@Include stdio.h second so it will see our own TRUE/FALSE
@
text
@d74 11
d90 1
d95 8
a102 3
	if (argc != 3) {
		printf("Usage is: %s <device> <nsector>\n", argv[0]);
		exit(1);
d117 15
a131 7
	bzero(sec, SECSZ);
	printf("Pre-allocate: "); fflush(stdout);
	for (x = 0; x < nsec; ++x) {
		fwrite(sec, SECSZ, 1, fp);
		/* Put a marker each 8K */
		if ((x % ((8*1024)/SECSZ)) == 0) {
			printf("."); fflush(stdout);
@


1.1
log
@Initial revision
@
text
@d6 1
a7 1
#include "../vstafs.h"
@
