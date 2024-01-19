head	1.16;
access;
symbols
	V1_3_1:1.14
	V1_3:1.14
	V1_2:1.13
	V1_1:1.13;
locks; strict;
comment	@ * @;


1.16
date	94.06.21.20.59.12;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.05.30.21.28.15;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.04.06.21.57.21;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.10.18.21.53.32;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.10.03.00.27.20;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.09.27.23.09.11;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.09.19.19.15.04;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.09.18.18.10.03;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.08.31.00.38.18;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.08.30.03.39.57;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.29.19.12.00;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.29.18.42.55;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.28.14.15.54;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.27.15.40.15;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.27.13.42.17;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.09.00.25;	author vandys;	state Exp;
branches;
next	;


desc
@Main definitions for VSTa filesystem
@


1.16
log
@Convert to openlog()
@
text
@#ifndef VSTAFS_H
#define VSTAFS_H
/*
 * vstafs.h
 *	Definitions for the VSTa-specific filesystem
 *
 * The VSTa filesystem has the following features:
 *	Hierarchical, acyclic directory structure
 *	28-character filenames
 *	Automatic file versioning
 *	Hardened filesystem semantics
 *	Extent-based storage allocation
 */
#include <sys/perm.h>
#include <sys/fs.h>

#define SECSZ (512)		/* Basic size of allocation units */
#define MAXEXT (32)		/* Max # extents in a file */
#define MAXNAMLEN (28)		/* Max chars in dir entry name */
#define EXTSIZ (128)		/* File growth increment */
				/*  ...must be power of 2! */
#define NCACHE (8*EXTSIZ)	/* Crank up if you have lots of users */
#define CORESEC (512)		/* Sectors to buffer in core at once */

/* Disk addresses, specified as a sector number */
typedef unsigned long daddr_t;

/* Conversion of units: bytes<->sectors */
#define btos(x) ((x) / SECSZ)
#define btors(x) (((x) + SECSZ-1) / SECSZ)
#define stob(x) ((x) * SECSZ)

/* Utility */
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) >= (y)) ? (x) : (y))

/*
 * The first sector of a filesystem
 */
#define BASE_SEC ((daddr_t)0)
struct fs {
	ulong fs_magic;		/* Magic number */
	ulong fs_size;		/* # sectors in filesystem */
	ulong fs_extsize;	/* Contiguous space allocated on extension */
	daddr_t fs_free;	/* Start of free list */
};
#define FS_MAGIC (0xDEADFACE)	/* Value for fs_magic */

/*
 * Next sector is root directory
 */
#define ROOT_SEC ((daddr_t)1)

/*
 * Sector after is first block of free list entries
 */
#define FREE_SEC ((daddr_t)2)

/*
 * The <start,len> pair describing file extents and contiguous
 * chunks on the free list.
 */
struct alloc {
	daddr_t a_start;	/* Starting sector # */
	ulong a_len;		/* Length, in sectors */
};

/*
 * A free list sector
 */
struct free {
	daddr_t f_next;		/* Next sector of free list */
	uint f_nfree;		/* # free in this sector */
#define NALLOC ((SECSZ-2*sizeof(ulong))/sizeof(struct alloc))
	struct alloc		/* Zero or more */
		f_free[NALLOC];
};

/*
 * A directory is simply a file structured as fs_dirent records
 */
struct fs_dirent {
	char fs_name[MAXNAMLEN];	/* Name */
	daddr_t fs_clstart;		/* Starting cluster # */
};

/*
 * The first part of a file is a description of the file and its
 * block allocation.  The file's contents follows.
 */
struct fs_file {
	daddr_t fs_prev;	/* Previous version of this file 0 */
	ulong fs_rev;		/* Revision # 4 */
	ulong fs_len;		/* File length in bytes 8 */
	ushort fs_type;		/* Type of file 12 */
	ushort fs_nlink;	/* # dir entries pointing to this 14 */
	struct prot		/* Protection on this file 16 */
		fs_prot;
	uint fs_owner;		/*  ...creator's UID 32 */
	uint fs_nblk;		/* # extents 36 */
	struct alloc		/* ...<start,off> tuples of extents */
		fs_blks[MAXEXT];	/* 288 */
	char fs_pad[24];	/* Pad to 32-byte boundary */
				/*  ...this keeps fs_dirent's aligned */
	char fs_data[0];	/* Data starts here */
};

/*
 * File types
 */
#define FT_FILE (1)
#define FT_DIR (2)

/* # bytes which reside at the tail of the file's information sector */
#define OFF_DATA (sizeof(struct fs_file))
#define OFF_SEC1 (SECSZ - ((int)(((struct fs_file *)0)->fs_data)))

/*
 * Structure of an open file in the filesystem
 */
struct openfile {
	daddr_t o_file;		/* 1st sector of file */
	ulong o_len;		/*  ...first extent's length */
	ulong o_hiwrite;	/* Highest file position written */
	uint o_refs;		/* # references */
};

/*
 * Our per-client data structure
 */
struct file {
	struct openfile		/* Current file open */
		*f_file;
	ulong f_pos;		/* Current file offset */
	struct perm		/* Things we're allowed to do */
		f_perms[PROCPERMS];
	uint f_nperm;
	uint f_perm;		/*  ...for the current f_file */
	long f_rename_id;	/* Transaction # for rename() */
	struct msg		/*  ...message for that transaction */
		f_rename_msg;
};

/*
 * Globals and such to keep us honest
 */
extern void read_sec(daddr_t, void *), write_sec(daddr_t, void *);
extern void read_secs(daddr_t, void *, uint),
	write_secs(daddr_t, void *, uint);
extern int blkdev;
extern void vfs_open(struct msg *, struct file *),
	vfs_read(struct msg *, struct file *),
	vfs_write(struct msg *, struct file *),
	vfs_remove(struct msg *, struct file *),
	vfs_stat(struct msg *, struct file *),
	vfs_wstat(struct msg *, struct file *),
	vfs_close(struct file *),
	vfs_fid(struct msg *, struct file *);
extern void init_buf(void), init_node(void), init_block(void);
extern void ref_node(struct openfile *), deref_node(struct openfile *);
extern struct openfile *get_node(daddr_t);
extern uint fs_perms(struct perm *, uint, struct openfile *);
extern struct buf *bmap(struct buf *, struct fs_file *,
	ulong, uint, char **, uint *);
extern struct fs_file *getfs(struct openfile *, struct buf **);
extern void cancel_rename(struct file *);
extern void vfs_rename(struct msg *, struct file *);

#endif /* VSTAFS_H */
@


1.15
log
@Syslog support
@
text
@a167 1
extern char vfs_sysmsg[];
@


1.14
log
@Add rename() support, pass 1
@
text
@d168 1
@


1.13
log
@Add WSTAT support
@
text
@d139 3
d166 2
@


1.12
log
@Add byte offset calculations, pad fs_file so that file data
starts on a 32-byte boundary.
@
text
@d153 1
@


1.11
log
@Record highest write so we can trim off excess preallocation
when we close written file.
@
text
@d92 6
a97 6
	daddr_t fs_prev;	/* Previous version of this file */
	ulong fs_rev;		/* Revision # */
	ulong fs_len;		/* File length in bytes */
	ushort fs_type;		/* Type of file */
	ushort fs_nlink;	/* # dir entries pointing to this */
	struct prot		/* Protection on this file */
d99 2
a100 2
	uint fs_owner;		/*  ...creator's UID */
	uint fs_nblk;		/* # extents */
d102 3
a104 1
		fs_blks[MAXEXT];
@


1.10
log
@A single buf extent shouldn't exhaust the cache!
@
text
@d122 1
@


1.9
log
@Add MIN/MAX
@
text
@d22 1
a22 1
#define NCACHE (64)		/* Crank up if you have lots of users */
@


1.8
log
@New arg to bmap()
@
text
@d33 4
@


1.7
log
@Remove redundant field
@
text
@d152 2
a153 1
extern struct buf *bmap(struct fs_file *, ulong, uint, char **, uint *);
@


1.6
log
@Convert accesses to fs_file to use the routine getfs()
@
text
@a87 1
	daddr_t fs_clstart;	/* Starting cluster # */
@


1.5
log
@Add bmap() prototype
@
text
@d154 1
@


1.4
log
@Further file creation/deletion stuff
@
text
@d153 1
@


1.3
log
@Clean up init prototypes
@
text
@d96 1
@


1.2
log
@Convert buffer resize to use only block addresses.  Do rest of
read/write support.
@
text
@d148 1
a148 1
extern void binit(void), fat_init(void), dir_init(void);
d150 1
a150 1
extern struct openfile *alloc_node(daddr_t);
@


1.1
log
@Initial revision
@
text
@d21 1
@
