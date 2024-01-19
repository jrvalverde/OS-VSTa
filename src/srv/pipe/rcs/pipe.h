head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.5
date	94.10.05.18.32.29;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.49.30;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.05.06.23.20.07;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.23.19.48.52;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.23.19.10.15;	author vandys;	state Exp;
branches;
next	;


desc
@All data declarations
@


1.5
log
@Merge in Dave Hudson's bug fixes, especially "foo | less"
function.
@
text
@#ifndef _PIPE_H
#define _PIPE_H
/*
 * tmpfs.h
 *	Data structures in temp filesystem
 *
 * PIPE is a VM-based FIFO buffer manager.  Its usual use is to open
 * /pipe/# (or wherever you mount the pipe manager) and receive a
 * new pipe.  Alternatively, /pipe/<number> will access an existing
 * pipe.  A pipe's "number" is its inum, from rstat().
 *
 * Internally, the pipe's "number" is simply its storage address.
 */
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/perm.h>
#include <llist.h>

/*
 * Structure of a pipe
 */
struct pipe {
	struct llist *p_entry;	/* Link into list of pipes */
	struct prot p_prot;	/* Protection of pipe */
	int p_refs;		/* # references */
	uint p_owner;		/* Owner UID */
	struct llist p_readers,	/* List of read requests pending */
		p_writers;	/*  ...writers */
	int p_nwrite;		/* # clients open for writing */
	int p_nread;		/* # clients open for reading */
};

/*
 * Our per-open-file data structure
 */
struct file {
	struct pipe	/* Current pipe open */
		*f_file;
	struct perm	/* Things we're allowed to do */
		f_perms[PROCPERMS];
	uint f_nperm;
	uint f_perm;	/*  ...for the current f_file */
	struct msg	/* For writes, segments of data */
		f_msg;	/*  for reads, just reply addr & count */
	struct llist	/* When request active, queue we're in */
		*f_q;
	uint f_pos;	/* Only for directory reads */
};

#define PIPE_CLOSED_FOR_READS -1

#endif /* _PIPE_H */
@


1.4
log
@Source reorg
@
text
@d30 1
d49 2
@


1.3
log
@Fiddle with reference and writer counting; they were wrong with
dup()'s of an open writer.  Also added some debug stuff to put
sanity checks on reference count values.
@
text
@d17 1
a17 1
#include <lib/llist.h>
@


1.2
log
@Add EOF on close of last writer
@
text
@d25 1
a25 1
	uint p_refs;		/* # references */
d29 1
a29 1
	uint p_nwrite;		/* # clients open for writing */
@


1.1
log
@Initial revision
@
text
@d29 1
@
