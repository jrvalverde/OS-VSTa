head	1.13;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.13
date	94.11.15.05.23.06;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.10.23.17.42.37;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.09.23.20.36.37;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.07.10.18.50.02;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.08.04.12.40;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.04.03.21.27.13;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.03.28.23.03.08;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.23.21.57.57;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.03.08.20.05.55;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.06.30.19.57.42;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.07.21.14.52;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.21.45.37;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Central definitions for hardware and software
@


1.13
log
@Pass node, not file.  dir_timestamp only needs node-level info.
@
text
@#ifndef _DOS_H
#define _DOS_H
/*
 * dos.h
 *	Definitions for DOS-type filesystem
 *
 * Throughout this filesystem server no attempt has been made to
 * insulate from the endianness of the processor.  It is assumed that
 * if you are running with DOS disks you're an Intel-type processor.
 * If you're on something else, you probably will make a DOS-ish
 * filesystem for yourself, with your own byte order, and enjoy the
 * savings in CPU time.  The exception is floppies; for those,
 * perhaps the mtools would suffice.  Or just use tar.
 */
#include <sys/types.h>
#include <sys/msg.h>
#include <time.h>

#define SECSZ (512)		/* Bytes in a sector */

/*
 * This represents the cluster allocation for a directory or file.
 * The c_clust field points to an array of unsigned short's which
 * are the clusters allocated.  It is malloc()'ed, and realloc()'ed
 * as needed to change storage allocation.
 */
struct clust {
	ushort *c_clust;	/* Clusters allocated */
	uint c_nclust;		/* # entries in c_clust */
};

/*
 * An open DOS file/dir has one of these.  It is thrown away on last close
 * of a file; it is kepts indefinitely for directories, unless the
 * directory itself is deleted.  In the future, it might be worth keeping
 * it for a while to see if it'll be needed again.
 */
struct node {
	uint n_type;		/* Type field */
	struct clust *n_clust;	/* Block allocation for this file */
	uint n_mode;		/* Protection bits */
	uint n_refs;		/* Current open files on us */
	uint n_flags;		/* Flags */
	struct node *n_dir;	/* Dir we exist within */
	uint n_slot;		/*  ...index within */

	/* For T_FILE only */
	ulong n_len;		/* Our byte length */

	/* For T_DIR only */
	struct hash		/* Hash for index->file mapping */
		*n_files;
};

/*
 * Values for n_type
 */
#define T_DIR 1		/* Directory */
#define T_FILE 2	/* File */
#define T_SYM 3		/* Symlink */

/*
 * Bits for n_flags
 */
#define N_DIRTY 1	/* Contents modified */
#define N_DEL 2		/* Node has been removed */

/*
 * Each open client has this state
 */
struct file {
	uint f_perm;		/* Things he can do */
	ulong f_pos;		/* Byte position in file */
	struct node *f_node;	/* Either a dosdir or a dosfile */
	long f_rename_id;	/* Transaction # for rename() */
	struct msg		/*  ...message for that transaction */
		f_rename_msg;
};

/*
 * A DOS directory entry
 */
struct directory {
	char name[8];		/* file name */
	char ext[3];		/* file extension */
	uint attr:8;		/* attribute byte */
	uchar reserved[10];	/* DOS reserved */
	uint time:16;		/* time stamp */
	uint date:16;		/* date stamp */
	uint start:16;		/* starting cluster number */
	uint size:32;		/* size of the file */
};

/*
 * Bits in "attr"
 */
#define DA_READONLY 0x01	/* Read only */
#define DA_HIDDEN 0x02		/* Hidden */
#define DA_SYSTEM 0x04		/* System */
#define DA_VOLUME 0x08		/* Volume label */
#define DA_DIR 0x10		/* Subdirectory */
#define DA_ARCHIVE 0x20		/* Needs archiving */

/*
 * Format of sector 0 in filesystem
 */
struct boot {
	uint jump1:16;		/* Jump to boot code 0 */
	uint jump2:8;
	char banner[8];		/* OEM name & version  3 */
	uint secsize0:8;	/* Bytes per sector hopefully 512  11 */
	uint secsize1:8;
	uint clsize:8;		/* Cluster size in sectors 13 */
	uint nrsvsect:16;	/* Number of reserved (boot) sectors 14 */
	uint nfat:8;		/* Number of FAT tables hopefully 2 16 */
	uint dirents0:8;	/* Number of directory slots 17 */
	uint dirents1:8;
	uint psect0:8;		/* Total sectors on disk 19 */
	uint psect1:8;
	uint descr:8;		/* Media descriptor=first byte of FAT 21 */
	uint fatlen:16;		/* Sectors in FAT 22 */
	uint nsect:16;		/* Sectors/track 24 */
	uint nheads:16;		/* Heads 26 */
	uint nhs:32;		/* number of hidden sectors 28 */
	uint bigsect:32;	/* big total sectors 32 */
};

/*
 * Parameters for block cache
 */
#define NCACHE (80)
extern uint clsize;
#define BLOCKSIZE (clsize)

/*
 * Values for FAT slots.  These are FAT16 values; FAT12 values are
 * mapped into these.
 */
#define FAT_RESERVED (0xFFF0)	/* Start of reserved value range */
#define FAT_DEFECT (0xFFF7)	/* Cluster w. defective block */
#define FAT_EOF (0xFFF8)	/* Start of EOF range */
#define FAT_END (0xFFFF)	/* End of reserved range */

/*
 * Node handling routines
 */
extern void rw_init(void);
extern struct node *rootdir;
extern struct node *do_lookup(struct node *, char *);
extern void ref_node(struct node *),
	deref_node(struct node *);

/*
 * Cluster handling routines
 */
extern void clust_init(void);
extern struct clust *alloc_clust(uint);
extern void free_clust(struct clust *);
extern void fat_sync(void);
extern int clust_setlen(struct clust *, ulong);
extern uint get_clust0(struct clust *);

/*
 * Block cache
 */
extern void *bget(int), *bdata(void *), bdirty(void *);
extern void binit(void), bsync(void), bfree(void *);

/*
 * Other routines
 */
extern int valid_fname(char *, int);
extern struct node *dir_look(struct node *, char *),
	*dir_newfile(struct file *, char *, int);
extern int dir_empty(struct node *);
extern void dir_remove(struct node *);
extern int dir_copy(struct node *, uint, struct directory *);
extern void dir_setlen(struct node *);
extern void root_sync(void), sync(void);
extern void fat_init(void), binit(void), dir_init(void);
extern void dos_open(struct msg *, struct file *),
	dos_close(struct file *),
	dos_rename(struct msg *, struct file *),
	cancel_rename(struct file *),
	dos_read(struct msg *, struct file *),
	dos_write(struct msg *, struct file *),
	dos_remove(struct msg *, struct file *),
	dos_stat(struct msg *, struct file *),
	dos_wstat(struct msg *, struct file *),
	dos_fid(struct msg *, struct file *);
extern void timestamp(struct directory *, time_t),
	dir_timestamp(struct node *, time_t);
extern int dir_set_type(struct file *, char *);
extern void dir_readonly(struct file *f, int);

/*
 * Global data
 */
extern int blkdev;
extern struct boot bootb;
extern uint dirents;
extern struct node *rootdir;

#endif /* _DOS_H */
@


1.12
log
@Implement read-only attribute.  Also fix flushing of
symlink type to disk.
@
text
@d192 1
a192 1
	dir_timestamp(struct file *, time_t);
@


1.11
log
@Add symlink support
@
text
@d194 1
@


1.10
log
@Add setting of mtime for DOS files
@
text
@d56 1
a56 2
 * Values for n_type.  The point is that this field is at the
 * same place for both, so you can tell one from the other.
d60 1
d193 1
@


1.9
log
@Increase buffer size
@
text
@d17 1
d189 1
d191 2
@


1.8
log
@Add renaming/moving of directories
@
text
@d130 1
a130 1
#define NCACHE (40)
@


1.7
log
@Keep message fields, not pointer to message (we reuse the
same message data on each received message)
@
text
@d146 4
a149 6
extern void rw_init(void);	/* Set up root directory */
extern struct node *rootdir;	/*  ...it's always here */
extern struct node		/* Look up name in directory */
	*do_lookup(struct node *, char *);
extern void			/* Add/remove a reference to a node */
	ref_node(struct node *),
d155 6
a160 8
extern void clust_init(void);	/* Bring FAT tables into memory */
extern struct clust		/* Allocate representation of chain */
	*alloc_clust(uint);
extern void			/*  ...free this representation */
	free_clust(struct clust *);
extern void fat_sync(void);	/* Sync FAT table to disk */
extern int			/* Set cluster allocation */
	clust_setlen(struct clust *, ulong);
@


1.6
log
@Fix -Wall warnings
@
text
@d16 1
d76 1
a76 1
		*f_rename_msg;
@


1.5
log
@Fix up handling of dir entry timestamps.  Clear fat_dirty
flag in a couple places.  Avoid a needless lseek() for the
common case.
@
text
@d73 3
d172 1
a172 1
 * Directory stuff
d174 1
d182 18
@


1.4
log
@Fix struct alignment, can't assume bitfields will fall on byte
boundary any more.
@
text
@d173 1
@


1.3
log
@Add flag to indicate deleted node
@
text
@d103 18
a120 14
	uint jump:24;		/* Jump to boot code */
	char banner[8];		/* OEM name & version */
	uint secsize:16;	/* Bytes per sector hopefully 512 */
	uint clsize:8;		/* Cluster size in sectors */
	uint nrsvsect:16;	/* Number of reserved (boot) sectors */
	uint nfat:8;		/* Number of FAT tables hopefully 2 */
	uint dirents:16;	/* Number of directory slots */
	uint psect:16;		/* Total sectors on disk */
	uint descr:8;		/* Media descriptor=first byte of FAT */
	uint fatlen:16;		/* Sectors in FAT */
	uint nsect:16;		/* Sectors/track */
	uint nheads:16;		/* Heads */
	uint nhs:32;		/* number of hidden sectors */
	uint bigsect:32;	/* big total sectors */
@


1.2
log
@Add some prototypes
@
text
@d64 1
@


1.1
log
@Initial revision
@
text
@d153 1
a153 1
extern void clust_sync(void);	/* Sync FAT table to disk */
d170 2
@
