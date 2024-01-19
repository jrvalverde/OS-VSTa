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
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.06.30.19.56.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.23.30.00;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.17.03.23;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.05.16.02.28;	author vandys;	state Exp;
branches;
next	;


desc
@Declarations for environment/string server
@


1.5
log
@Source reorg
@
text
@#ifndef _ENV_H
#define _ENV_H
/*
 * env.h
 *	Data structures for the environment server
 *
 * The env server provides a hierarchical shared name space.  At
 * the lowest level of the tree are process-private parts of the
 * name space.  Upper levels are shared by successively larger groups
 * of processes.  A name is looked up by starting at the lowest node
 * and searching upward until it is found.
 */
#include <sys/msg.h>
#include <sys/perm.h>
#include <sys/param.h>
#include <llist.h>

/*
 * An open file
 */
struct file {
	struct node *f_node,	/* Current node we address */
		*f_home;	/* "home" node */
	struct perm		/* Our abilities */
		f_perms[PROCPERMS];
	int f_nperm;		/*  ...# active */
	off_t f_pos;		/* Position in internal node */
	int f_mode;		/* Abilities WRT current node */
	struct file		/* Members sharing f_home */
		*f_forw,*f_back;
};

/*
 * A string value shared by one or more nodes
 */
struct string {
	uint s_refs;		/* # nodes using */
	char *s_val;		/* String value */
};

/*
 * A node in our name tree
 */
struct node {
	struct prot n_prot;	/* Protection of this node */
	char n_name[NAMESZ];	/* Name of node */
	ushort n_flags;		/* Flags */
	struct string *n_val;	/*  ...if not, string value for name */
	struct llist n_elems;	/* Linked list of elements under node */
	uint n_refs;		/* # references to this node */
	struct llist *n_list;	/* Our place in our parent's list */
	struct node *n_up;	/*  ...out parent */
	uint n_owner;		/* Owner UID # */
};

/*
 * Flag bits
 */
#define N_INTERNAL 1		/* Is a "directory" */

/*
 * An oft-asked question
 */
#define DIR(n) ((n)->n_flags & N_INTERNAL)

/*
 * Some function prototypes
 */
extern void deref_val(struct string *),
	ref_val(struct string *);
extern struct string *alloc_val(char *);
extern struct node *node_cow(struct node *);
extern void env_open(struct msg *, struct file *),
	env_read(struct msg *, struct file *, uint),
	env_write(struct msg *, struct file *, uint),
	env_remove(struct msg *, struct file *),
	env_stat(struct msg *, struct file *),
	env_wstat(struct msg *, struct file *);
extern struct node *alloc_node(struct file *),
	*clone_node(struct node *);
extern void ref_node(struct node *), deref_node(struct node *),
	remove_node(struct node *);

#endif /* _ENV_H */
@


1.4
log
@Fix GCC warnings
@
text
@d16 1
a16 1
#include <lib/llist.h>
@


1.3
log
@Add UID tag, get from client
@
text
@d78 1
a78 2
	env_wstat(struct msg *, struct file *),
	env_seek(struct msg *, struct file *);
@


1.2
log
@Add prototypes plus rework environment inheritance
@
text
@d53 1
@


1.1
log
@Initial revision
@
text
@d13 1
a13 1
#include <sys/types.h>
d22 2
a23 1
	struct node *f_node;	/* Current node we address */
d29 2
d47 1
a47 1
	ushort n_internal;	/* Internal node? */
d56 10
d71 12
@
