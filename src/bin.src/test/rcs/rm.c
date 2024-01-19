head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1
	V1_1:1.1;
locks; strict;
comment	@ * @;


1.2
date	94.04.07.00.12.02;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.10.06.23.58.17;	author vandys;	state Exp;
branches;
next	;


desc
@Remove files & dirs
@


1.2
log
@Don't return error when -f
@
text
@/*
 * rm.c
 *	A simple file/dir removal program
 */
#include <stdio.h>
#include <stat.h>
#include <dirent.h>

/* Option flags */
static int rflag, fflag;

/* Forward dec'ls */
static int cleardir(void);
static void do_remove(char *);

/* Error count */
static int errs = 0;

/*
 * cleardir()
 *	Remove all entries in current directory
 *
 * Returns 1 on failure of dir itself, 0 otherwise.
 */
static int
cleardir(void)
{
	DIR *dir;
	struct dirent *de;

	dir = opendir(".");
	if (dir == 0) {
		return(1);
	}
	for (de = readdir(dir); de; de = readdir(dir)) {
		do_remove(de->d_name);
	}
	closedir(dir);
	return(0);
}

/*
 * do_remove()
 *	Do actual removal for a named entry
 */
static void
do_remove(char *n)
{
	struct stat sb;

	/*
	 * Common/simple case
	 */
	if (unlink(n) >= 0) {
		return;
	}

	/*
	 * Figure out what it is
	 */
	if (stat(n, &sb) < 0) {
		if (!fflag) {
			perror(n);
			errs = 1;
		}
		return;
	}

	/*
	 * Things other than directories, give up now.
	 */
	if ((sb.st_mode & S_IFMT) != S_IFDIR) {
		if (!fflag) {
			perror(n);
			errs = 1;
		}
		return;
	}

	/*
	 * If not -r, complain & bail
	 */
	if (!rflag) {
		if (!fflag) {
			fprintf(stderr, "%s: is a directory\n", n);
			errs = 1;
		}
		return;
	}

	/*
	 * Descend, remove
	 */
	if (chdir(n) < 0) {
		if (!fflag) {
			perror(n);
			errs = 1;
		}
		return;
	}
	if (cleardir()) {
		if (!fflag) {
			perror(n);
			errs = 1;
		}
	}
	if (chdir("..") < 0) {
		perror("Return from subdir");
		abort();
	}

	/*
	 * Now try to clear the dir one more time
	 */
	if (unlink(n) < 0) {
		if (!fflag) {
			perror(n);
			errs = 1;
		}
		return;
	}
}

main(int argc, char **argv)
{
	int x, forced = 0;

	/*
	 * Parse leading options
	 */
	for (x = 1; (x < argc) && !forced; ++x) {
		char *p;

		if (argv[x][0] != '-') {
			break;
		}
		for (p = argv[x]+1; *p; ++p) {
			switch (*p) {
			case 'r': rflag = 1; break;
			case 'f': fflag = 1; break;
			case '-': forced = 1; break;
			}
		}
	}

	/*
	 * Process remaining arguments on command line
	 */
	for ( ; x < argc; ++x) {
		do_remove(argv[x]);
	}
	return(errs);
}
@


1.1
log
@Initial revision
@
text
@d16 3
d62 1
a62 1
		if (!fflag)
d64 2
d73 1
a73 1
		if (!fflag)
d75 2
d84 1
a84 1
		if (!fflag)
d86 2
d95 1
a95 1
		if (!fflag)
d97 2
d104 1
d116 1
a116 1
		if (!fflag)
d118 2
d152 1
@
