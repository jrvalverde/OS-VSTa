head	1.6;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.6
date	94.09.26.17.14.49;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.09.23.20.38.37;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.20.00.23.47;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.26.18.45.20;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.15.09.01;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.48.29;	author vandys;	state Exp;
branches;
next	;


desc
@Mount point handling definitions
@


1.6
log
@Fix extern def for get_mntinfo()
@
text
@#ifndef _MNTTAB_H
#define _MNTTAB_H
/*
 * mnttab.h
 *	Data structures for mount table
 *
 * The mount table is organized as an array of entries describing
 * mount point strings.  Hanging off of each entry is then a linked
 * list of directories mounted at this name.  A pathname lookup
 * thus entails finding the longest matching path from the mnttab,
 * and then trying the entries under this mnttab slot in order until
 * an open is successful or you run out of entries to try.
 */
#include <sys/types.h>

/*
 * The mount table is an array of these
 */
struct mnttab {
	char *m_name;			/* Name mounted under */
	int m_len;			/* strlen(m_name) */
	struct mntent *m_entries;	/* Entries to lookup for this point */
};

/*
 * One or more of these per mount point
 */
struct mntent {
	port_t m_port;		/* Port to ask */
	struct mntent *m_next;	/* Next in list */
};

/*
 * Routines for manipulating mounts
 */
extern int mount(char *, char *), mountport(char *, port_t);
extern int umount(char *, port_t);
extern void init_mount(char *);
extern ulong __mount_size(void);
extern void __mount_save(char *);
extern char *__mount_restore(char *);
extern void __get_mntinfo(int *, struct mnttab **);

#endif /* _MNTTAB_H */
@


1.5
log
@Add procedural interfaces to all global C library data
@
text
@d42 1
a42 1
extern void __get_mntinfo(int *, struct mnttab *);
@


1.4
log
@Add prototype for init_mount() function in libc
@
text
@d42 1
@


1.3
log
@Add prototypes for save/restore interface
@
text
@d38 1
@


1.2
log
@Fix name of structure field; name was historical, and is
quite misleading now.
@
text
@d38 3
@


1.1
log
@Initial revision
@
text
@d29 1
a29 1
	port_t m_fd;		/* Port to ask */
@
