head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.4
date	94.12.21.05.35.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.15.05.01.54;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.26.45;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.20.00.19.53;	author vandys;	state Exp;
branches;
next	;


desc
@Routines for messing with passwd entries
@


1.4
log
@Fix string scan
@
text
@/*
 * passwd.c
 *	Routines for getting VSTa-style account information
 *
 * Format in password file is:
 *	account:password:uid:gid:name:baseperm:home:env:shell
 */
#include <passwd.h>
#include <stdio.h>
#include <std.h>

extern void parse_perm();

#define DEFSHELL "/vsta/bin/sh"
#define DEFHOME "/vsta"
#define DEFENV "/env"

/*
 * load_groups()
 *	Given GID, perhaps add some more abilities
 */
static void
load_groups(struct uinfo *u)
{
	FILE *fp;
	char buf[128];
	char *p;
	int nperm = 1;

	/*
	 * Read group file
	 */
	if ((fp = fopen(GROUP, "r")) == 0) {
		return;
	}
	while (fgets(buf, sizeof(buf), fp)) {
		/*
		 * Leave if there's no more slots to fill
		 */
		if (nperm >= PROCPERMS) {
			break;
		}

		/*
		 * Get rid of trailing \n, skip to GID field
		 */
		buf[strlen(buf)-1] = '\0';
		p = strchr(buf, ':');
		if (p == 0) {
			break;
		}
		++p;

		/*
		 * Compare GID to our own GID
		 */
		if (atoi(p) != u->u_gid) {
			continue;
		}

		/*
		 * Match.  Move to first of n ability fields
		 */
		p = strchr(p, ':');
		if (p == 0) {
			break;
		}
		++p;

		/*
		 * While there are more fields, add them to the user
		 */
		while (p) {
			char *q;

			/*
			 * Null-terminate current, record next
			 */
			q = strchr(p, ':');
			if (q) {
				*q++ = '\0';
			}

			/*
			 * Parse permission into struct
			 */
			parse_perm(&u->u_perms[nperm], p);
			u->u_perms[nperm].perm_uid = u->u_uid;

			/*
			 * Advance to next
			 */
			nperm += 1;
			p = q;
		}

		/*
		 * Assume there's only one entry for a given
		 * GID.
		 */
		break;
	}
	fclose(fp);
}

/*
 * fillin()
 *	Given account record, fill in uinfo struct
 */
static void
fillin(char *rec, struct uinfo *u)
{
	char *p;
	extern void zero_ids();

	/*
	 * Default to something harmless
	 */
	u->u_uid = u->u_gid = 2;
	strcpy(u->u_passwd, "*");
	strcpy(u->u_home, DEFHOME);
	strcpy(u->u_shell, DEFSHELL);
	strcpy(u->u_env, DEFENV);
	zero_ids(u->u_perms, PROCPERMS);

	/*
	 * Password
	 */
	p = strchr(rec, ':');
	if (p) {
		*p++ = '\0';
	}
	strcpy(u->u_passwd, rec);
	if ((rec = p) == 0) {
		return;
	}

	/*
	 * UID
	 */
	u->u_uid = atoi(rec);
	rec = strchr(rec, ':');
	if (rec == 0) {
		return;
	}
	rec++;

	/*
	 * GID
	 */
	u->u_gid = atoi(rec);
	rec = strchr(rec, ':');
	if (rec == 0) {
		return;
	}
	rec++;

	/*
	 * Skip name
	 */
	rec = strchr(rec, ':');
	if (rec == 0) {
		return;
	}
	rec++;

	/*
	 * Parse permission string
	 */
	p = strchr(rec, ':');
	if (p) {
		*p++ = '\0';
	}
	parse_perm(&u->u_perms[0], rec);
	u->u_perms[0].perm_uid = u->u_uid;
	if ((rec = p) == 0) {
		return;
	}

	/*
	 * If there are further groups to be added because of
	 * GID, add them here.
	 */
	load_groups(u);

	/*
	 * Parse home
	 */
	p = strchr(rec, ':');
	if (p) {
		*p++ = '\0';
	}
	strcpy(u->u_home, rec);
	if ((rec = p) == 0) {
		return;
	}

	/*
	 * Parse environment
	 */
	p = strchr(rec, ':');
	if (p) {
		*p++ = '\0';
	}
	strcpy(u->u_env, rec);
	if ((rec = p) == 0) {
		return;
	}

	/*
	 * The rest is the shell
	 */
	strcpy(u->u_shell, rec);
}

/*
 * getuinfo_name()
 *	Get account information given name
 *
 * Returns 0 on success, 1 on failure
 */
getuinfo_name(char *name, struct uinfo *u)
{
	FILE *fp;
	char *p, buf[80];

	/*
	 * Open password file
	 */
	fp = fopen(PASSWD, "r");
	if (fp == 0) {
		return(1);
	}

	/*
	 * Get lines out of file until EOF or match
	 */
	while (fgets(buf, sizeof(buf), fp)) {
		buf[strlen(buf)-1] = '\0';

		/*
		 * Chop off first field--account name
		 */
		p = strchr(buf, ':');
		if (p == 0) {
			continue;	/* Malformed */
		}

		/*
		 * Match?
		 */
		*p++ = '\0';
		if (!strcmp(name, buf)) {
			fclose(fp);
			strcpy(u->u_acct, name);
			fillin(p, u);
			return(0);
		}
	}
	fclose(fp);
	return(1);
}
@


1.3
log
@Add group lists of abilities
@
text
@d73 1
a73 1
		while (p && *p) {
@


1.2
log
@new UID code
@
text
@d19 88
d181 2
a182 1
	 * XXX add more perms based on group
d184 1
@


1.1
log
@Initial revision
@
text
@a24 1
	int x;
d26 1
d31 1
a31 1
	u->u_gid = 2;
d36 1
a36 3
	for (x = 0; x < PROCPERMS; ++x) {
		u->u_perms[x].perm_len = PERMLEN+1;
	}
d87 1
d91 4
@
