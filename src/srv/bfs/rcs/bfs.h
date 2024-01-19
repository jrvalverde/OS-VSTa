head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.04.10.19.54.29;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.03.08.20.04.21;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.38.07;	author vandys;	state Exp;
branches;
next	;


desc
@Boot Filesystem definitions
@


1.3
log
@Cleanup, add time stamp support, add rename() support
@
text
@#ifndef _BFS_H
#define _BFS_H


/*
 * Filename:	bfs.h
 * Developed:	Dave Hudson <dave@@humbug.demon.co.uk>
 * Originated:	Andy Valencia
 * Last Update: 8th April 1994
 * Implemented:	GNU GCC version 2.5.7
 *
 * Description: Data structures in boot filesystem
 *
 * BFS is a very simple contiguous-allocation filesystem.  Free blocks
 * are always consumed at the end of the filesystem; when these are
 * exhausted, the filesystem is compacted and all current free space
 * coalesced at the end.
 *
 * BFS is not written as an interactive filesystem; it is single-
 * threaded.
 */


#include <sys/types.h>
#include <sys/fs.h>


#define BLOCKSIZE 1024		/* Size of blocks in filesystem */
#define BFSNAMELEN 44		/* Max chars in filename (-1 for null) */
#define SMAGIC 0x121314		/* Magic number to tell a BFS superblock */
#define NCACHE 32		/* Number of blocks to cache in memory */
#define MAXINODE 64		/* Number of inodes open at once */
#define MINDATABLOCKS 16	/* Minimum number of data blocks in fs */
#define ROOTINODE 0		/* Special inode number for root */
#define I_FREE -1		/* Free inode reference */


/*
 * Convert bytes to # block which would hold that
 */
#define BLOCKS(bytes) \
	((bytes + (BLOCKSIZE - 1)) / BLOCKSIZE)


/*
 * Structure of directory entries
 */
struct dirent {
	uint d_inum;		/* Inode number */
	uint d_start;		/* First block in the file */
	uint d_len;		/* Length of file in bytes */
	uint d_ctime;		/* File creation time */
	uint d_mtime;		/* Last modification time */
	char d_name[BFSNAMELEN];
				/* Name of file */
};


/*
 * Structure of the first data in the filesystem.  This structure does not
 * really need a block to itself, but I want to make life extremely simple
 * for the system boot loader.
 */
struct super {
	uint s_magic;		/* Magic # ID for superblock */
	uint s_blocks;		/* Total blocks in filesystem */
	uint s_supstart;	/* Start block number for superblock info */
	uint s_supblocks;	/* Number of blocks of superblock info */
	uint s_dirstart;	/* Start block number for directory info */
	uint s_dirblocks;	/* Number of blocks of directory info */
	uint s_datastart;	/* Start block number for data */
	uint s_datablocks;	/* Number of data blocks */
	uint s_free;		/* Number currently unused */
	uint s_nextfree;	/* Next free block */
	uint s_ndirents;	/* Number of directory entries */
	uint s_direntsize;	/* Size in bytes of a directory entry */
};


/*
 * Our per-open-file data structure
 */
struct file {
	struct inode *f_inode;	/* Current inode */
	uint f_pos;		/* Current file offset */
	int f_write;		/* Flag if this open allowed to write */
	long f_rename_id;	/* Transaction number for rename */
	struct msg f_rename_msg;
				/* Message for that rename transactions */
};


/*
 * Our inode (per-file) info.  Note that we also track any free space after
 * the inode (since we have contiguous allocation), and keep references to
 * the inodes that describe the files either side of this one
 */
struct inode {
	uint i_num;		/* Inode number */
	uint i_refs;		/* Number of open files on the node */
	uint i_start;		/* Start block of data handled by inode */
	uint i_blocks;		/* Number of blocks managed by inode*/
	uint i_fsize;		/* Size in bytes of file data */
	uint i_next;		/* Inode num for next file in the fs */
	uint i_prev;		/* Inode num for previous file in the fs */
	uint i_dirblk;		/* FS block number for dir entry */
	uint i_diroff;		/* Offest in dir block for dir entry */
	uint i_ctime;		/* File creation time */
	uint i_mtime;		/* File modification time */
	char i_name[BFSNAMELEN];
				/* Name of the file */
};


/*
 * Definitions for block.c - block I/O and buffer management code
 */
extern void bdirty(void *bp);
extern void *bget(int blkno);
extern void *bdata(void *bp);
extern void bfree(void *bp);
extern void binit(void);
extern void bsync(void);


/*
 * Definitions for filectrl.c - file/dir tracking code
 */
extern void ino_ref(struct inode *i);
extern void ino_deref(struct inode *i);
extern struct inode *ino_lookup(char *name);
extern int ino_copy(int inum, struct inode *i);
extern struct inode *ino_find(uint inum);
extern struct inode *ino_new(char *name);
extern void ino_clear(struct inode *i);
extern void ino_dirty(struct inode *i);
extern void ino_init(void);
extern void blk_trunc(struct inode *i);
extern int blk_alloc(struct inode *i, uint newsize);


/*
 * Definitions for open.c - file open/close/delete code
 */
extern void bfs_open(struct msg *m, struct file *f);
extern void bfs_close(struct file *f);
extern void bfs_remove(struct msg *m, struct file *f);
extern void bfs_rename(struct msg *m, struct file *f);
extern void cancel_rename(struct file *f);


/*
 * Definitions for rw.c - file read/write code
 */
extern void bfs_write(struct msg *m, struct file *f);
extern void bfs_read(struct msg *m, struct file *f);


/*
 * Definitions for stat.c - filesystem stat code
 */
extern void bfs_stat(struct msg *m, struct file *f);


#endif /* _BFS_H */
@


1.2
log
@Rev boot filesystem per work from Dave Hudson
@
text
@d9 1
a9 1
 * Last Update: 11th February 1994
d29 5
a33 4
#define BFSNAMELEN 20		/* Max chars in filename (-1 for null) */
#define SMAGIC 0x121314		/* Magic # to tell a BFS superblock */
#define NCACHE 32		/* # blocks to cache in memory */
#define MAXINODE 64		/* # inodes open at once */
d52 2
d87 3
d108 2
d148 2
@


1.1
log
@Initial revision
@
text
@d3 2
d6 7
a12 2
 * bfs.h
 *	Data structures in boot filesystem
d22 2
d27 1
d29 1
a29 2
#define NDIRBLOCKS 4		/* # blocks used for directory entries */
#define NAMELEN 16		/* Max chars in filename (-1 for null) */
d33 7
a39 3
#define ROOTINO \
	((struct inode *)0)	/* Special inode ptr for root */
/* Convert bytes to # block which would hold that */
d41 3
a43 1
	((bytes + (BLOCKSIZE-1)) / BLOCKSIZE)
d48 2
a49 3
	char d_name[NAMELEN];	/* Name of file */
	uint d_inum;		/* Inode # */
	uint d_start;		/* Starting block # for entry */
d51 2
d55 1
d57 3
a59 1
 * Structure of first block in filesystem
d63 7
a69 1
	uint s_nblocks;		/* Total blocks in filesystem */
d72 2
a73 2
	struct dirent		/* Directory entries start here */
		s_dir[1];
a75 8
/*
 * A way to talk about the position of a directory entry on the
 * block device.
 */
struct dirmap {
	uint d_blkno;	/* Block # of dir slot */
	uint d_off;	/* Offset within that block */
};
d81 3
a83 4
	struct inode	/* Current inode */
		*f_inode;
	uint f_pos;	/* Current file offset */
	int f_write;	/* Flag if this open allowed to write */
d86 1
d88 3
a90 1
 * Our per-file info
d93 11
a103 4
	uint i_num;	/* Inode # */
	struct dirmap	/* Corresponding directory entry info */
		i_dir;
	uint i_refs;	/* # open files on the node */
d105 49
@
