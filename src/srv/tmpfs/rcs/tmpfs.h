head	1.5;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	95.01.10.05.36.28;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.11.16.19.37.51;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.09.30.22.55.03;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.28.29;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.12.20.51.08;	author vandys;	state Exp;
branches;
next	;


desc
@Data structures
@


1.5
log
@Change blocksize to 2K, not 8K.  This chews up way less memory
when there are a lot of very small files (during build of libc.shl,
in particular).
@
text
@#ifndef _TMPFS_H
#define _TMPFS_H
/*
 * tmpfs.h
 *	Data structures in temp filesystem
 *
 * TMPFS is a VM-based filesystem, which thus does not survive
 * reboots.  It is flat, and stores file contents into a chain
 * of malloc()-allocated blocks.
 *
 * Directory entries are found using a symbol table lookup.  Each
 * open file contains a hash which maps block number into the
 * block's virtual address.
 */
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/perm.h>

#define BLOCKSIZE 2048		/* Size of blocks in filesystem */

/*
 * Structure of an open file in the filesystem
 */
struct openfile {
	char *o_name;		/* Name of file */
	struct hash *o_blocks;	/* Map to blocks (0-based) */
	ulong o_len;		/* Length in bytes */
	struct llist *o_entry;	/* Link into list of names in filesystem */
	struct prot o_prot;	/* Protection of file */
	uint o_refs;		/* # references */
	uint o_owner;		/* Owner UID */
	int o_deleted;		/* Auto-delete on last close */
};

/*
 * Our per-open-file data structure
 */
struct file {
	struct openfile	/* Current file open */
		*f_file;
	ulong f_pos;	/* Current file offset */
	struct perm	/* Things we're allowed to do */
		f_perms[PROCPERMS];
	int f_nperm;
	int f_perm;	/*  ...for the current f_file */
};

extern void tmpfs_open(struct msg *, struct file *),
	tmpfs_read(struct msg *, struct file *),
	tmpfs_write(struct msg *, struct file *),
	tmpfs_remove(struct msg *, struct file *),
	tmpfs_stat(struct msg *, struct file *),
	tmpfs_close(struct file *),
	tmpfs_wstat(struct msg *, struct file *),
	tmpfs_fid(struct msg *, struct file *);

#endif /* _TMPFS_H */
@


1.4
log
@Add FS_FID support so we can run a.out's from /tmp
@
text
@d19 1
a19 1
#define BLOCKSIZE 8192		/* Size of blocks in filesystem */
@


1.3
log
@Allow busy files to be marked for deletion
@
text
@d48 9
@


1.2
log
@Add UID tag for nodes, get from client
@
text
@d32 1
@


1.1
log
@Initial revision
@
text
@d31 1
@
