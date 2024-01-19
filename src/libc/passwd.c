/*
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
