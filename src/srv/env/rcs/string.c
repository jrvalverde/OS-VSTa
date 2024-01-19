head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.2
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.05.16.02.28;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of strings.  Designed to allow nodes to share strings,
though not currently used.  Might be used in future to support
fork()-like duplication of lowest level of a process' environment.
@


1.2
log
@Source reorg
@
text
@/*
 * string.c
 *	Handling of struct string
 */
#include "env.h"
#include <std.h>

/*
 * ref_val()
 *	Add reference to the value
 */
void
ref_val(struct string *s)
{
	if (!s) {
		return;
	}
	s->s_refs += 1;
}

/*
 * deref_val()
 *	Remove reference to value, free node on last reference
 */
void
deref_val(struct string *s)
{
	if (!s) {
		return;
	}
	if ((s->s_refs -= 1) == 0) {
		free(s->s_val);
		free(s);
	}
}

/*
 * alloc_val()
 *	Allocate a new string node
 */
struct string *
alloc_val(char *p)
{
	struct string *s;

	s = malloc(sizeof(struct string));
	if (s == 0) {
		return(0);
	}
	s->s_refs = 1;
	if ((s->s_val = malloc(strlen(p)+1)) == 0) {
		free(s);
		return(0);
	}
	return(s);
}
@


1.1
log
@Initial revision
@
text
@d5 1
a5 1
#include <env/env.h>
@
