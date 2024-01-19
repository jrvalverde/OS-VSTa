head	1.7;
access;
symbols
	V1_3_1:1.7
	V1_3:1.5
	V1_2:1.5
	V1_1:1.5
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.7
date	94.04.19.05.29.40;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.04.19.00.26.44;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.43.53;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.12.20.57.15;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.09.17.13.26;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.03.23.18.06;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.03.23.15.48;	author vandys;	state Exp;
branches;
next	;


desc
@Exit group handling.  Used to support concept of a "parent"
process being able to wait for children, and receiving various
information about children as they exit.
@


1.7
log
@Missing deref on parent detach
@
text
@/*
 * exitgrp.c
 *	Routines for dealing with exitgrp structures
 */
#include <sys/proc.h>
#include <sys/assert.h>
#include <sys/malloc.h>
#include <alloc.h>

/*
 * parent_exitgrp()
 *	Tell PID of parent of exit group, or -1
 */
pid_t
parent_exitgrp(struct exitgrp *e)
{
	pid_t pid;

	(void)p_lock(&e->e_lock, SPL0);
	if (e->e_parent) {
		pid = e->e_parent->p_pid;
	} else {
		pid = -1;
	}
	v_lock(&e->e_lock, SPL0);
	return(pid);
}

/*
 * alloc_exitgrp()
 *	Allocate an exitgrp, perhaps with a parent
 */
struct exitgrp *
alloc_exitgrp(struct proc *parent)
{
	struct exitgrp *e;

	e = MALLOC(sizeof(struct exitgrp), MT_EXITGRP);
	e->e_parent = parent;
	e->e_stat = 0;
	e->e_refs = (parent ? 1 : 0);
	init_lock(&e->e_lock);
	init_sema(&e->e_sema); set_sema(&e->e_sema, 0);
	return(e);
}

/*
 * ref_exitgrp()
 *	Add a reference to an exitgrp
 */
void
ref_exitgrp(struct exitgrp *e)
{
	(void)p_lock(&e->e_lock, SPL0);
	e->e_refs += 1;
	ASSERT_DEBUG(e->e_refs > 0, "ref_exitgrp: overflow");
	v_lock(&e->e_lock, SPL0);
}

/*
 * deref_exitgrp()
 *	Remove a reference from an exitgrp
 *
 * Frees memory on last reference.  Assumes that the parent of the
 * exitgrp has already disassociated itself.
 */
void
deref_exitgrp(struct exitgrp *e)
{
	p_lock(&e->e_lock, SPL0);
	ASSERT_DEBUG(e->e_refs > 0, "deref_exitgrp: no refs");
	e->e_refs -= 1;
	if (e->e_refs > 0) {
		v_lock(&e->e_lock, SPL0);
		return;
	}
	ASSERT_DEBUG(e->e_parent == 0, "deref_exitgrp: !ref parent");
	ASSERT_DEBUG(e->e_stat == 0, "deref_exitgrp: stat");
	v_lock(&e->e_lock, SPL0);	/* for per-CPU lock counting */
	FREE(e, MT_EXITGRP);
}

/*
 * noparent_exitgrp()
 *	Disassociate parent from exit group
 *
 * Done on exit() of parent.  Cleans up any remaining status messages
 * and then marks exitgrp as having no parent.  This call removes
 * one reference from the exitgrp as well.
 */
void
noparent_exitgrp(struct exitgrp *e)
{
	struct exitst *es, *esn;

	p_lock(&e->e_lock, SPL0);
	ASSERT_DEBUG(e->e_parent, "noparent_exitgrp: no parent");

	/*
	 * Remove parent pointer
	 */
	e->e_parent = 0;

	/*
	 * Gather any remaining status messages.  We could
	 * actually do this after releasing the lock, but
	 * I think this way begs for less trouble.
	 */
	es = e->e_stat;
	e->e_stat = 0;
	v_lock(&e->e_lock, SPL0);

	/*
	 * Free the status messages
	 */
	while (es) {
		esn = es->e_next;
		FREE(es, MT_EXITST);
		es = esn;
	}

	/*
	 * Drop the parental reference
	 */
	deref_exitgrp(e);
}

/*
 * post_exitgrp()
 *	Post an exit message to the exit group
 */
void
post_exitgrp(struct exitgrp *e, struct proc *p, int code)
{
	struct exitst *es;

	/*
	 * Take a quick, unlocked look.  If it appears 0, it
	 * can never change back.  We handle the case of it
	 * *becoming* 0 as we run below.
	 */
	if (e->e_parent == 0) {
		return;
	}

	/*
	 * Get a status message, fill it in
	 */
	es = MALLOC(sizeof(struct exitst), MT_EXITST);
	es->e_pid = p->p_pid;
	es->e_code = code;
	es->e_usr = p->p_usr;
	es->e_sys = p->p_sys;
	strcpy(es->e_event, p->p_event);

	/*
	 * Queue.  Zero our pointer if it was queued, leave it non-zero
	 * if our parent has departed.
	 */
	p_lock(&e->e_lock, SPL0);
	if (e->e_parent) {
		es->e_next = e->e_stat;
		e->e_stat = es;
		es = 0;
		v_sema(&e->e_sema);
	}
	v_lock(&e->e_lock, SPL0);

	/*
	 * If we raced, free the (unqueued) message
	 */
	if (es) {
		FREE(es, MT_EXITST);
	}
}

/*
 * wait_exitgrp()
 *	Let parent wait for a child, if any
 */
struct exitst *
wait_exitgrp(struct exitgrp *e, int block)
{
	struct exitst *es;

	ASSERT_DEBUG(e->e_parent, "wait_exitgrp: !parent");
retry:
	p_lock(&e->e_lock, SPL0);

	/*
	 * A status message awaits.  Take it.
	 */
	if (es = e->e_stat) {
		e->e_stat = es->e_next;

	/*
	 * There are no children; return NULL without sleeping
	 */
	} else if ((e->e_refs == 1) || !block) {
		es = 0;
	/*
	 * Wait for a child
	 */
	} else {
		p_sema_v_lock(&e->e_sema, PRILO, &e->e_lock);

		/*
		 * We now have a message to remove
		 */
		p_lock(&e->e_lock, SPL0);
		es = e->e_stat;
		ASSERT_DEBUG(es, "wait_exitgrp: stat out of synch");
		e->e_stat = es->e_next;
		v_lock(&e->e_lock, SPL0);
		return(es);
	}

	/*
	 * Release lock and figure out where we stand
	 */
	v_lock(&e->e_lock, SPL0);

	/*
	 * No status messages and no children.  Just return
	 * an error.
	 */
	if (es == 0) {
		return(0);
	}

	/*
	 * Otherwise we got a message.  We p_sema() here to keep
	 * the sema count right; we shouldn't actually block, since
	 * presumably the sema count was bumped by whoever put the
	 * status message on the exitgrp.
	 *
	 * We couldn't just p_sema() at the top; that wouldn't
	 * handle the case of "all children gone".
	 */
	p_sema(&e->e_sema, PRIHI);
	return(es);
}
@


1.6
log
@Convert to MALLOC
@
text
@d121 5
@


1.5
log
@Source reorg
@
text
@d7 1
d38 1
a38 1
	e = malloc(sizeof(struct exitgrp));
d80 1
a80 1
	free(e);
d118 1
a118 1
		free(es);
d144 1
a144 1
	es = malloc(sizeof(struct exitst));
d168 1
a168 1
		free(es);
@


1.4
log
@Add interface for getting paren't ID
@
text
@d7 1
a7 1
#include <lib/alloc.h>
@


1.3
log
@Add non-blocking for waits()
@
text
@d10 19
@


1.2
log
@Get rid of debugging printf's
@
text
@d129 1
d157 1
a157 1
wait_exitgrp(struct exitgrp *e)
d174 1
a174 1
	} else if (e->e_refs == 1) {
@


1.1
log
@Initial revision
@
text
@a111 1
	printf("post_exitgrp grp 0x%x code %d\n", e, code);
a160 1
	printf("wait_exitgrp grp 0x%x\n", e);
@
