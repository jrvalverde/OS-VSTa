head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.3
	V1_2:1.3;
locks; strict;
comment	@ * @;


1.5
date	94.07.10.18.25.18;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.18.22.22.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.12.23.21.01.45;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.12.09.06.20.45;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.11.28.03.06.59;	author vandys;	state Exp;
branches;
next	;


desc
@Copy program
@


1.5
log
@Clean up failure conditions
@
text
@/*
 * cp.c
 *	Copy files
 */
#include <fcntl.h>
#include <stdio.h>
#include <std.h>
#include <stat.h>
#include <dirent.h>

static char *buf;		/* I/O buffer--malloc()'ed */
#define BUFSIZE (16*1024)	/* I/O buffer size */

/*
 * Part of source filename not copied.  When you copy, the destination
 * gets only the basename of the path (or basename on down, for -r).
 * So "cp /x/y /z" does not create file "/z/x/y", only "/z/y".
 */
static int path_off;

static int errs = 0;		/* Count of errors */
static int rflag = 0,		/* Recursive copy */
	vflag = 0;		/* Verbose */

static void cp_file(char *, char *);

/*
 * fisdir()
 *	Version of isdir() given file descriptor
 */
static int
fisdir(int fd)
{
	struct stat sb;

	if (fstat(fd, &sb) < 0) {
		return(0);
	}
	return((sb.st_mode & S_IFMT) == S_IFDIR);
}

/*
 * isdir()
 *	Tell if named file is a directory
 */
static int
isdir(char *n)
{
	int x, fd;

	fd = open(n, O_READ);
	if (fd < 0) {
		return(0);
	}
	x = fisdir(fd);
	close(fd);
	return(x);
}

/*
 * cp_recurse()
 *	Recursively copy contents of one dir into another
 */
static void
cp_recurse(char *src, char *dest)
{
	DIR *dir;
	struct dirent *de;
	char *srcname, *destname;

	if ((dir = opendir(src)) == 0) {
		perror(src);
		return;
	}
	(void)mkdir(dest);
	while (de = readdir(dir)) {
		srcname = malloc(strlen(src)+strlen(de->d_name)+2);
		if (srcname == 0) {
			perror(src);
			exit(1);
		}
		sprintf(srcname, "%s/%s", src, de->d_name);
		destname = malloc(strlen(dest)+strlen(de->d_name)+2);
		if (destname == 0) {
			perror(dest);
			exit(1);
		}
		sprintf(destname, "%s/%s", dest, de->d_name);
		cp_file(srcname, destname);
		free(destname); free(srcname);
	}
	closedir(dir);
}

/*
 * cp_file()
 *	Copy one file to another
 */
static void
cp_file(char *src, char *dest)
{
	int in, out, x;

	/*
	 * Access source
	 */
	if ((in = open(src, O_READ)) < 0) {
		perror(src);
		errs += 1;
		return;
	}

	/*
	 * If source is a directory, bomb or start recursive copy
	 */
	if (fisdir(in)) {
		close(in);
		if (!rflag) {
			fprintf(stderr, "%s: is a directory\n", src);
			errs += 1;
			return;
		}
		cp_recurse(src, dest);
		return;
	}

	if (vflag) {
		printf("%s -> %s\n", src, dest);
	}
	if ((out = open(dest, O_WRITE|O_CREAT, 0666)) < 0) {
		perror(dest);
		errs += 1;
		close(in);
		return;
	}
	while ((x = read(in, buf, BUFSIZE)) > 0) {
		if (write(out, buf, x) < 0) {
			perror(dest);
			errs++;
			close(out);
			close(in);
			return;
		}
	}
	close(out);
	close(in);
}

/*
 * cp_dir()
 *	Copy a file into a directory, using same name
 */
static void
cp_dir(char *src, char *destdir)
{
	char *dest;

	dest = malloc((strlen(src) - path_off) + strlen(destdir) + 2);
	if (dest == 0) {
		perror(destdir);
		errs += 1;
		return;
	}
	sprintf(dest, "%s/%s", destdir, src+path_off);
	cp_file(src, dest);
	free(dest);
}

/*
 * usage()
 *	Give usage message & exit
 */
static void
usage(void)
{
	fprintf(stderr,
"Usage is: cp <src> <dest>\nor\tcp <src1> [ <src2> ... ] <destdir>\n");
	exit(1);
}

main(int argc, char **argv)
{
	int lastdir;
	char *dest;

	/*
	 * Get an I/O buffer
	 */
	buf = malloc(BUFSIZE);
	if (buf == 0) {
		perror("cp");
		exit(1);
	}

	while ((argc > 1) && (argv[1][0] == '-')) {
		char *p;

		/*
		 * Process switches
		 */
		for (p = argv[1]+1; *p; ++p) {
			switch (*p) {
			case 'r':
				rflag = 1; break;
			case 'v':
				vflag = 1; break;
			default:
				fprintf(stderr, "Unknown flag: %c\n",
					*p);
				exit(1);
			}
		}

		/*
		 * Make the option(s) disappear
		 */
		argc -= 1;
		argv[1] = argv[0];
		argv += 1;
	}

	/*
	 * Sanity check argument count
	 */
	if (argc < 3) {
		usage();
	}

	/*
	 * Parse rest of args.  Support both:
	 *	cp <src> <dest>
	 * and	cp <src1> <src2> ... <dest-dir>
	 */
	dest = argv[argc-1];
	lastdir = isdir(dest);
	if ((argc > 3) || lastdir) {
		int x;

		if (!lastdir) {
			usage();
		}
		for (x = 1; x < argc-1; ++x)  {
			char *p;

			if (p = strrchr(argv[x], '/')) {
				path_off = (p - argv[x])+1;
			} else {
				path_off = 0;
			}
			cp_dir(argv[x], dest);
		}
		return(errs);
	}
	cp_file(argv[1], argv[2]);
	return(errs);
}
@


1.4
log
@Missing return value for main()
@
text
@d137 7
a143 1
		write(out, buf, x);
@


1.3
log
@Add rest of -r
@
text
@d246 1
a246 1
		return;
@


1.2
log
@Add recursive flag
@
text
@a3 3
 *
 * Does not do -r, because -r hoses links and symlinks.  Use tar
 * or cpio.
d9 1
d14 7
d22 4
a25 1
static int rflag = 0;		/* -r flag specified */
d61 35
d117 1
d124 1
d127 3
d152 1
a152 1
	dest = malloc(strlen(src) + strlen(destdir) + 2);
d158 1
a158 1
	sprintf(dest, "%s/%s", destdir, src);
d189 22
a210 5
	/*
	 * Parse "-r"; allow recursive copy
	 */
	if ((argc > 1) && !strcmp(argv[1], "-r")) {
		rflag = 1;
d237 7
@


1.1
log
@Initial revision
@
text
@d17 1
d20 33
d61 3
d69 13
a126 15
/*
 * isdir()
 *	Tell if named file is a directory
 */
static int
isdir(char *n)
{
	struct stat sb;

	if (stat(n, &sb) < 0) {
		return(0);
	}
	return((sb.st_mode & S_IFMT) == S_IFDIR);
}

d140 14
d159 1
a159 1
	 * Parse args.  Support both:
@
