head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.4
date	93.11.16.02.50.17;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.10.19.02.34.55;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.31.00.06.43;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.57.03;	author vandys;	state Exp;
branches;
next	;


desc
@Linked list object
@


1.4
log
@Source reorg
@
text
@/*
 * llist.c
 *	Routines for doing linked list operations
 */
#include <llist.h>

/*
 * ll_init()
 *	Initialize list head
 */
void
ll_init(struct llist *l)
{
	l->l_forw = l->l_back = l;
#ifdef DEBUG
	l->l_data = 0;
#endif
}

/*
 * ll_insert()
 *	Insert datum at given place in list
 *
 * Returns new node, or 0 for failure
 */
struct llist *
ll_insert(struct llist *l, void *d)
{
	struct llist *lnew;
	extern void *malloc();

	lnew = malloc(sizeof(struct llist));
	if (lnew == 0)
		return 0;
	lnew->l_data = d;

	lnew->l_forw = l;
	lnew->l_back = l->l_back;
	l->l_back->l_forw = lnew;
	l->l_back = lnew;
	return(lnew);
}

/*
 * ll_delete()
 *	Remove node from linked list, free storage
 */
void
ll_delete(struct llist *l)
{
	l->l_back->l_forw = l->l_forw;
	l->l_forw->l_back = l->l_back;
#ifdef DEBUG
	l->l_forw = l->l_back = 0;
	l->l_data = 0;
#endif
	free(l);
}

/*
 * ll_movehead()
 *	Move head of list to place within
 */
void
ll_movehead(struct llist *head, struct llist *l)
{
	l->l_back->l_forw = l->l_forw;
	l->l_forw->l_back = l->l_back;
	l->l_forw = head->l_forw;
	l->l_back = head;
	head->l_forw->l_back = l;
	head->l_forw = l;
}
@


1.3
log
@Move the actual element, not the list head--this was breaking
the organization of age lists, since it would change all member's
position relative to the list head!
@
text
@d5 1
a5 1
#include <lib/llist.h>
@


1.2
log
@Missing patch from back-pointer
@
text
@d67 5
a71 2
	head->l_back->l_forw = head->l_forw;
	head->l_forw->l_back = head->l_back;
a72 3
	head->l_back = l->l_back;
	l->l_back->l_forw = head;
	l->l_back = head;
@


1.1
log
@Initial revision
@
text
@d71 1
@
