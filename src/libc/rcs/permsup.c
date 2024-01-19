head	1.7;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5
	V1_1:1.5
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.7
date	94.12.21.05.35.50;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.04.06.21.59.04;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.14.01.08.32;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.24.19.11.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.24.00.18.52;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.20.00.20.23;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.57.20;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of in-core parts of permission/protection handling
@


1.7
log
@Fix perm calculation
@
text
@/*
 * permsup.c
 *	Routines for looking at permission/protection structures
 */
#include <sys/types.h>
#include <sys/perm.h>
#include <std.h>
#include <ctype.h>

/*
 * perm_step()
 *	Calculate granted bits for a single permission struct
 */
static
perm_step(struct perm *perm, struct prot *prot)
{
	int x, y, granted;

	/*
	 * Everybody gets the first ones
	 */
	granted = prot->prot_default;

	/*
	 * Keep adding while the label matches
	 */
	for (x = 0; x < prot->prot_len; ++x) {
		/*
		 * This label dominates
		 */
		if (x >= PERM_LEN(perm)) {
			for (y = x; y < prot->prot_len; ++y) {
				granted |= prot->prot_bits[y];
			}
			return(granted);
		}

		/*
		 * Do we still match?
		 */
		if (prot->prot_id[x] != perm->perm_id[x]) {
			return(granted);
		}

		/*
		 * Yes, add bits and keep going
		 */
		granted |= prot->prot_bits[x];
	}
	return(granted);
}

/*
 * perm_calc()
 *	Given a permission and a protection array, return permissions
 */
perm_calc(struct perm *perms, int nperms, struct prot *prot)
{
	int x, granted;

	granted = 0;
	for (x = 0; x < nperms; ++x) {
		if (!PERM_ACTIVE(&perms[x])) {
			continue;
		}
		granted |= perm_step(&perms[x], prot);
	}
	return(granted);
}

/*
 * zero_ids()
 *	Zero out the permissions array for a process
 */
void
zero_ids(struct perm *perms, int nperms)
{
	int x;

	bzero(perms, sizeof(struct perm) * nperms);
	for (x = 0; x < nperms; ++x) {
		PERM_NULL(&perms[x]);
	}
}

/*
 * perm_dominates()
 *	Tell if a permission dominates another
 *
 * A permission dominates another if (1) it is shorter and matches
 * to its length, or (2) is identical except that it is disabled.
 */
perm_dominates(struct perm *us, struct perm *target)
{
	uint x;
	struct perm p;

	/*
	 * Always consider target as if it would be enabled
	 */
	p = *target;
	PERM_ENABLE(&p);

	/*
	 * Always allowed to shut off a slot
	 */
	if (!PERM_ACTIVE(&p)) {
		return(1);
	}

	/*
	 * We're shorter?
	 */
	if (PERM_ACTIVE(us) && (us->perm_len <= p.perm_len)) {
		/*
		 * Match leading values
		 */
		for (x = 0; x < PERM_LEN(us); ++x) {
			if (us->perm_id[x] != p.perm_id[x]) {
				break;
			}
		}

		/*
		 * Yup, we matched for all our digits
		 */
		if (x >= PERM_LEN(us)) {
			return(1);
		}
	}

	/*
	 * See if we're a match except for being disabled
	 */
	if (!PERM_DISABLED(us) || PERM_DISABLED(target)) {
		return(0);
	}
	if (PERM_LEN(us) != PERM_LEN(&p)) {
		return(0);
	}
	for (x = 0; x < PERM_LEN(us); ++x) {
		if (us->perm_id[x] != p.perm_id[x]) {
			return(0);
		}
	}
	return(1);
}
@


1.6
log
@File rename, to support libusr->libc merge
@
text
@d33 1
a33 1
				granted |= prot->prot_bits[x];
@


1.5
log
@Move non-kernel stuff to permpr
@
text
@d2 1
a2 1
 * perm.c
@


1.4
log
@Off-by-one in how much to scan from perm string
@
text
@d8 1
a83 28
}

/*
 * parse_perm()
 *	Parse a numeric dotted string into a struct perm
 */
void
parse_perm(struct perm *p, char *s)
{
	int len;

	/*
	 * Special case--no digits, Mr. Superuser
	 */
	if (*s == '\0') {
		p->perm_len = 0;
		return;
	}

	/*
	 * Parse dot-separated numbers
	 */
	len = 0;
	p->perm_id[len++] = atoi(s);
	while (s = strchr(s, '.')) {
		p->perm_id[len++] = atoi(++s);
	}
	p->perm_len = len;
@


1.3
log
@Use PERM_LEN macro
@
text
@d30 1
a30 1
		if (x > perm->perm_len) {
@


1.2
log
@Use macros for fiddling struct perm.  Add more routines
for handling permissions.
@
text
@d141 1
a141 1
	if (us->perm_len <= p.perm_len) {
d145 1
a145 1
		for (x = 0; x < us->perm_len; ++x) {
d154 1
a154 1
		if (x >= us->perm_len) {
@


1.1
log
@Initial revision
@
text
@d5 1
d7 1
d62 1
a62 1
		if (perms[x].perm_len > PERMLEN) {
d81 1
a81 1
		perms[x].perm_len = PERMLEN+1;
d83 91
@
