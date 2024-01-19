head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.2
date	93.02.25.21.21.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.57.08;	author vandys;	state Exp;
branches;
next	;


desc
@Linked list object definitions
@


1.2
log
@Add some macros to let our users code look nicer
@
text
@#ifndef _LLIST_H
#define _LLIST_H
/*
 * llist.h
 *	Data structure for doing doubly-linked lists
 */

/*
 * Structure of both list head (l_data not used) and list elements
 */
struct llist {
	struct llist *l_forw,
		*l_back;
	void *l_data;
};

/*
 * LL_EMPTY(), LL_NEXT()
 *	Tell if linked list is empty, who is next
 */
#define LL_EMPTY(l) ((l)->l_forw == (l))
#define LL_NEXT(l) ((l)->l_forw)

/*
 * ll_init()
 *	Initialize list head
 */
void ll_init(struct llist *);

/*
 * ll_insert()
 *	Insert datum at given place in list
 */
struct llist *ll_insert(struct llist *, void *);

/*
 * ll_delete()
 *	Remove node from linked list, free storage
 */
void ll_delete(struct llist *);

/*
 * ll_movehead()
 *	Move the list head to another place within the list
 *
 * This is needed to keep linked aging lists "fair" by making the
 * scan not keep considering the same things towards the front
 * of he list.
 */
void ll_movehead(struct llist *, struct llist *);

#endif /* _LLIST_H */
@


1.1
log
@Initial revision
@
text
@d18 7
@
