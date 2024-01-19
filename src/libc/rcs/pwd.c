head	1.6;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.6
date	95.02.02.04.37.08;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	95.01.10.05.21.54;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.06.07.23.01.22;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.50.35;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.05.03.20.43.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.04.14.01.10.54;	author vandys;	state Exp;
branches;
next	;


desc
@Password database interface functions
@


1.6
log
@Fix getpwnam(), didn't fill hash on first call, didn't use
hash_foreach() correctly
@
text
@/*
 * pwd.c
 *	Group file functions
 */
#include <sys/perm.h>
#include <pwd.h>
#include <stdio.h>
#include <hash.h>
#include <std.h>
#include <string.h>

static struct hash *uidhash = 0;	/* Mapping GID->struct passwd */

/*
 * field()
 *	Return indexed field from colon-separated list
 */
static char *
field(char *buf, int idx, char *deflt)
{
	char *p, *q;
	int x, len;
	static char *fldval = 0;

	/*
	 * Walk forward to indexed field
	 */
	p = buf;
	for (x = 0; x < idx; ++x) {
		p = strchr(p, ':');
		if (p == 0) {
			return(deflt);
		}
		++p;
	}

	/*
	 * Find terminator
	 */
	for (q = p+1; *q && (*q != ':'); ++q)
		;

	/*
	 * Calculate length, get static string to hold value
	 */
	len = q-p;
	fldval = realloc(fldval, len+1);
	if (fldval == 0) {
		return(deflt);
	}

	/*
	 * Put null-terminated value in place, return it
	 */
	bcopy(p, fldval, len);
	fldval[len] = '\0';
	return(fldval);
}

/*
 * fill_hash()
 *	One-time read of passwd file into local cache
 */
static void
fill_hash(void)
{
	FILE *fp;
	char buf[256], *p;
	struct passwd *pw;

	/*
	 * Access passwd file
	 */
	if ((fp = fopen("/vsta/etc/passwd", "r")) == 0) {
		return;
	}

	/*
	 * Allocate hash
	 */
	uidhash = hash_alloc(16);
	if (uidhash == 0) {
		abort();
	}

	/*
	 * Read lines
	 */
	buf[sizeof(buf)-1] = '\0';
	while (fgets(buf, sizeof(buf)-1, fp)) {
		/*
		 * Trim newline
		 */
		buf[strlen(buf)-1] = '\0';

		/*
		 * Allocate new passwd
		 */
		pw = malloc(sizeof(struct passwd));
		if (pw == 0) {
			break;
		}

		/*
		 * Fill in
		 */
		pw->pw_name = strdup(field(buf, 0, "???"));
		pw->pw_uid = atoi(field(buf, 2, "2"));
		pw->pw_gid = atoi(field(buf, 3, "2"));
		pw->pw_dir = strdup(field(buf, 6, "/vsta"));
		pw->pw_shell = strdup(field(buf, 8, "/vsta/bin/testsh"));

		/*
		 * Add to hash
		 */
		if (hash_insert(uidhash, pw->pw_uid, pw)) {
			/*
			 * XXX we lose the strdup()'ed fields, but no
			 * easy way to tell if they were the default
			 * values or strdup()'ed.
			 */
			free(pw);
			break;
		}
	}
	fclose(fp);
}

/*
 * getpwuid()
 *	Get struct passwd given UID
 */
struct passwd *
getpwuid(uid_t uid)
{
	/*
	 * Fill hash table once only
	 */
	if (uidhash == 0) {
		fill_hash();
	}

	/*
	 * Look up, return result
	 */
	return(hash_lookup(uidhash, uid));
}

/*
 * Encapsulates arguments to foreach function
 */
struct hasharg {
	char *name;
	struct passwd *passwd;
};

/*
 * namecheck()
 *	Check for match on name, end foreach when find it
 */
static
namecheck(uid_t uid, struct passwd *pw, struct hasharg *ha)
{
	if (!strcmp(pw->pw_name, ha->name)) {
		ha->passwd = pw;
		return(1);
	}
	return(0);
}

/*
 * getpwnam()
 *	Get struct passwd given passwd name
 */
struct passwd *
getpwnam(char *name)
{
	struct hasharg ha;

	/*
	 * Fill hash table once only
	 */
	if (uidhash == 0) {
		fill_hash();
	}
	ha.passwd = 0;
	ha.name = name;
	hash_foreach(uidhash, namecheck, &ha);
	return(ha.passwd);
}

/*
 * getuid()
 *	Get UID from first permission record
 */
uid_t
getuid(void)
{
	struct perm me;

	if (perm_ctl(0, 0, &me) < 0) {
		return(0);
	}
	return(me.perm_uid);
}

/*
 * getlogin()
 *	Get UID from first permission record, map to a name
 */
char *
getlogin(void)
{
	struct passwd *pw;

	pw = getpwuid(getuid());
	if (pw == 0) {
		return(0);
	}
	return(pw->pw_name);
}

/*
 * getgid()
 *	Get group ID for current user
 */
gid_t
getgid(void)
{
	struct passwd *pw;

	pw = getpwuid(getuid());
	if (pw == 0) {
		return(0);
	}
	return(pw->pw_gid);
}
@


1.5
log
@Add getuid() and getgid()
@
text
@d162 1
a162 1
namecheck(struct passwd *pw, struct hasharg *ha)
d180 6
@


1.4
log
@Add getlogin()
@
text
@d187 15
a207 1
	struct perm me;
d210 2
a211 1
	if (perm_ctl(0, 0, &me) < 0) {
d214 13
a226 1
	pw = getpwuid(me.perm_uid);
d230 1
a230 1
	return(pw->pw_name);
@


1.3
log
@Source reorg
@
text
@d5 1
d184 20
@


1.2
log
@Fix bugs w. read-in
@
text
@d7 1
a7 1
#include <lib/hash.h>
@


1.1
log
@Initial revision
@
text
@d39 1
a39 1
	for (q = p+1; q && (*q != ':'); ++q)
d45 2
a46 2
	len = (q-p)+1;
	fldval = realloc(fldval, len);
d54 2
a55 2
	bcopy(p, fldval, len-1);
	p[len] = '\0';
d75 8
@
