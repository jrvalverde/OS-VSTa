head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.3
date	93.04.14.01.08.51;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.24.22.27.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.57.32;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of printout of permission/protection stuff.
@


1.3
log
@Move perm parsing here, add symbolic support
@
text
@/*
 * permpr.c
 *	Routines for printing permission/protection structures
 */
#include <sys/perm.h>
#include <std.h>
#include <string.h>

/*
 * perm_print()
 *	Print out owner/bits stuff the way stat() wants it
 */
char *
perm_print(struct prot *prot)
{
	static char buf[PERMLEN*4*2+12];
	char buf2[16];
	int x;
	char *p;

	sprintf(buf, "perm=");
	p = buf;
	for (x = 0; x < prot->prot_len; ++x) {
		p = p+strlen(p);
		if (x > 0)
			strcat(p, "/");
		sprintf(buf2, "%d", prot->prot_id[x]);
		strcat(p, buf2);
	}
	strcat(p, "\nacc=");
	sprintf(buf2, "%d", prot->prot_default);
	strcat(p, buf2);
	for (x = 0; x < prot->prot_len; ++x) {
		p = p+strlen(p);
		sprintf(buf2, "/%d", prot->prot_bits[x]);
		strcat(p, buf2);
	}
	strcat(p, "\n");
	return(buf);
}

/*
 * perm_set()
 *	Set the prot_id/prot_bits fields from a string
 *
 * Returns number of fields parsed out of the string
 */
perm_set(unsigned char *field, char *val)
{
	int x, nfield = 0;
	char *p;

	/*
	 * Parse numbers separated by '/'s
	 */
	for (x = 0; val && *val && (x < PERMLEN); ++x) {
		for (p = val; *p && (*p != '/'); ++p)
			;
		field[nfield++] = atoi(val);
		if (*p) {
			val = p+1;
		} else {
			val = p;
		}
	}

	/*
	 * Fill trailing fields with 0
	 */
	for (x = nfield; x < PERMLEN; ++x)
		field[x] = 0;

	/*
	 * Return # fields filled in
	 */
	return(nfield);
}

/*
 * parse_perm()
 *	Parse a numeric dotted string into a struct perm
 *
 * Also handles names from /vsta/etc/ids.
 */
void
parse_perm(struct perm *p, char *s)
{
	int len;
	char *sn;

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
	while (s) {
		/*
		 * Find end of field, null-terminate if needed
		 */
		sn = strchr(s, '.');
		if (sn) {
			*sn++ = '\0';
		}

		/*
		 * Parse number or id from ids file
		 */
		if (isdigit(s[0])) {
			p->perm_id[len] = atoi(s);
		} else {
			p->perm_id[len] =
				look_id(s, p->perm_id, len);
		}
		len += 1;
		s = sn;
	}
	p->perm_len = len;
}
@


1.2
log
@Add trailing newline
@
text
@d6 2
d77 48
@


1.1
log
@Initial revision
@
text
@d36 1
@
