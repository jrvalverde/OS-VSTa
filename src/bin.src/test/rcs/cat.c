head	1.4;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.4
date	94.10.06.04.17.21;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.10.01.03.32.30;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.07.10.18.25.18;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.26.18.44.38;	author vandys;	state Exp;
branches;
next	;


desc
@cat(1) program, from UCB
@


1.4
log
@Convert to <getopt.h>
@
text
@/*
 * cat.c
 *
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kevin Fall.
 */
#include <stdio.h>
#include <std.h>
#include <ctype.h>
#include <fcntl.h>
#include <stat.h>
#include <getopt.h>

static int bflag, eflag, nflag, sflag, tflag, vflag;
static int rval;
static char *filename;

static void cook_args(), cook_buf(), raw_args(), raw_cat();

int
main(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "benstuv")) != EOF)
		switch (ch) {
		case 'b':
			bflag = nflag = 1;	/* -b implies -n */
			break;
		case 'e':
			eflag = vflag = 1;	/* -e implies -v */
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			tflag = vflag = 1;	/* -t implies -v */
			break;
		case 'u':
			setbuf(stdout, (char *)NULL);
			break;
		case 'v':
			vflag = 1;
			break;
		case '?':
			(void)fprintf(stderr,
			    "usage: cat [-benstuv] [-] [file ...]\n");
			exit(1);
		}
	argv += optind;

	if (bflag || eflag || nflag || sflag || tflag || vflag) {
		cook_args(argv);
	} else {
		raw_args(argv);
	}
	if (fclose(stdout)) {
		perror("stdout");
	}
	exit(rval);
}

static void
cook_args(argv)
	char **argv;
{
	FILE *fp;

	fp = stdin;
	filename = "stdin";
	do {
		if (*argv) {
			if (!strcmp(*argv, "-"))
				fp = stdin;
			else if (!(fp = fopen(*argv, "r"))) {
				perror(*argv);
				++argv;
				continue;
			}
			filename = *argv++;
		}
		cook_buf(fp);
		if (fp != stdin)
			(void)fclose(fp);
	} while (*argv);
}

static void
cook_buf(fp)
	FILE *fp;
{
	int ch, gobble, line, prev;

	line = gobble = 0;
	for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
		if (prev == '\n') {
			if (ch == '\n') {
				if (sflag) {
					if (!gobble && putchar(ch) == EOF)
						break;
					gobble = 1;
					continue;
				}
				if (nflag && !bflag) {
					(void)fprintf(stdout, "%6d\t", ++line);
					if (ferror(stdout))
						break;
				}
			} else if (nflag) {
				(void)fprintf(stdout, "%6d\t", ++line);
				if (ferror(stdout))
					break;
			}
		}
		gobble = 0;
		if (ch == '\n') {
			if (eflag)
				if (putchar('$') == EOF)
					break;
		} else if (ch == '\t') {
			if (tflag) {
				if (putchar('^') == EOF || putchar('I') == EOF)
					break;
				continue;
			}
		} else if (vflag) {
			if (!isascii(ch)) {
				if (putchar('M') == EOF || putchar('-') == EOF)
					break;
				ch = toascii(ch);
			}
			if (iscntrl(ch)) {
				if (putchar('^') == EOF ||
				    putchar(ch == '\177' ? '?' :
				    ch | 0100) == EOF)
					break;
				continue;
			}
		}
		if (putchar(ch) == EOF)
			break;
	}
	if (ferror(fp)) {
		perror("");
		clearerr(fp);
	}
	if (ferror(stdout)) {
		perror("stdout");
	}
}

static void
raw_args(char **argv)
{
	int fd;

	fd = fileno(stdin);
	filename = "stdin";
	do {
		if (*argv) {
			if (!strcmp(*argv, "-")) {
				fd = fileno(stdin);
			} else if ((fd = open(*argv, O_RDONLY, 0)) < 0) {
				perror(*argv);
				++argv;
				continue;
			}
			filename = *argv++;
		}
		raw_cat(fd);
		if (fd != fileno(stdin)) {
			(void)close(fd);
		}
	} while (*argv);
}

static void
raw_cat(int rfd)
{
	int nr, nw, off, wfd;
	static int bsize;
	static char *buf;
	struct stat sbuf;

	wfd = fileno(stdout);
	if (!buf) {
		if (fstat(wfd, &sbuf) < 0) {
			perror(filename);
			return;
		}
		bsize = MAX(sbuf.st_blksize, 1024);
		if (!(buf = malloc((uint)bsize))) {
			perror("malloc");
			return;
		}
	}
	while ((nr = read(rfd, buf, bsize)) > 0) {
		for (off = 0; off < nr; nr -= nw, off += nw) {
			if ((nw = write(wfd, buf + off, nr)) < 0) {
				perror("stdout");
				return;
			}
		}
	}
	if (nr < 0) {
		perror(filename);
	}
}
@


1.3
log
@Don't define optind for ourselves
@
text
@d15 1
@


1.2
log
@Clean up failure conditions
@
text
@a24 1
	extern int optind;
@


1.1
log
@Initial revision
@
text
@d16 3
a18 3
int bflag, eflag, nflag, sflag, tflag, vflag;
int rval;
char *filename;
d20 1
a20 1
void cook_args(), cook_buf(), raw_args(), raw_cat();
d22 2
a23 3
main(argc, argv)
	int argc;
	char **argv;
d69 1
a69 1
void
d94 1
a94 1
void
d158 2
a159 3
void
raw_args(argv)
	char **argv;
d183 2
a184 3
void
raw_cat(rfd)
	int rfd;
d193 1
a193 1
		if (fstat(wfd, &sbuf)) {
d195 1
d200 1
d207 1
@
