head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.5
	V1_1:1.5
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.5
date	93.11.16.02.51.47;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.23.19.49.44;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.23.27.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.42.19;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.11.59;	author vandys;	state Exp;
branches;
next	;


desc
@Namer definitions
@


1.5
log
@Source reorg
@
text
@#ifndef _NAMER_H
#define _NAMER_H
/*
 * namer.h
 *	Data structures and values for talking to the system namer
 *
 * The namer is responsible for mapping names into port addresses
 * so you can msg_connect() to them.
 */
#include <sys/msg.h>

port_name namer_find(char *);
int namer_register(char *, port_name);

#ifdef _NAMER_H_INTERNAL
#include <sys/types.h>
#include <sys/perm.h>
#include <sys/param.h>
#include <llist.h>

/*
 * An open file
 */
struct file {
	struct node *f_node;	/* Current node we address */
	struct perm		/* Our abilities */
		f_perms[PROCPERMS];
	int f_nperm;		/*  ...# active */
	off_t f_pos;		/* Position in internal node */
	int f_mode;		/* Abilities WRT current node */
};

/*
 * A node in our name tree
 */
struct node {
	struct prot n_prot;	/* Protection of this node */
	char n_name[NAMESZ];	/* Name of node */
	ushort n_internal;	/* Internal node? */
	port_name n_port;	/*  ...if not, port name for leaf */
	struct llist n_elems;	/* Linked list of elements under node */
	uint n_refs;		/* # references to this node */
	struct llist *n_list;	/* Our place in our parent's list */
	uint n_owner;		/* Owner UID */
};

#endif /* _NAMER_H_INTERNAL */

#endif /* _NAMER_H */
@


1.4
log
@Fix type dec'l for parm
@
text
@d19 1
a19 1
#include <lib/llist.h>
@


1.3
log
@Add UID tag
@
text
@d13 1
a13 1
int namer_register(char *, port_t);
@


1.2
log
@Delete obsolete argument for namer_find()
@
text
@d44 1
@


1.1
log
@Initial revision
@
text
@d12 1
a12 1
port_name namer_find(char *, int);
@
