head	1.4;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.4
date	94.05.30.00.48.26;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.05.30.00.47.08;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.09.28.19.34.36;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.09.51;	author vandys;	state Exp;
branches;
next	;


desc
@Dump little C program to make Makefile's
@


1.4
log
@Inhibit \r\n under DOS (works fine under VSTa anyway)
@
text
@/*
 * config.c
 *	Poor man's makefile maker
 */
#include <stdio.h>

#define COLS (50)		/* Max # cols used for OBJS list */

extern void *malloc(), *realloc();

char *objs = 0;

main()
{
	FILE *fp, *fpmk, *fpobj;
	int c, l, col = 0;
	char buf[128], dir[128];

	/*
	 * Open object list file.  We need this because of DOS'
	 * completely lame command length limit.
	 */
	if ((fpobj = fopen("objs", "wb")) == NULL) {
		perror("objs");
		exit(1);
	}

	/*
	 * Open output file
	 */
	if ((fpmk = fopen("makefile", "wb")) == NULL) {
		perror("makefile");
		exit(1);
	}

	/*
	 * Copy over the invariant part
	 */
	if ((fp = fopen("make.head", "r")) == NULL) {
		perror("make.head");
		exit(1);
	}
	while ((c = getc(fp)) != EOF)
		putc(c, fpmk);
	fclose(fp);

	/*
	 * Process file database
	 */
	if ((fp = fopen("files", "r")) == NULL) {
		perror("files");
		exit(1);
	}
	while (fgets(buf, sizeof(buf), fp)) {
		if ((buf[0] == '\n') || (buf[0] == '#'))
			continue;
		l = strlen(buf)-1;	/* Trim '\n' */
		buf[l] = '\0';

		/*
		 * If on to new directory, update dir variable
		 */
		if (buf[l-1] == ':') {
			buf[l-1] = '\0';
			strcpy(dir, buf);
			continue;
		}

		/*
		 * Otherwise generate a dependency
		 */
		if (buf[l-1] == 'c') {
			buf[l-2] = '\0';
			fprintf(fpmk,
"%s.o: ../%s/%s.c\n\t$(CC) $(CFLAGS) -c ../%s/%s.c\n\n",
				buf, dir, buf, dir, buf);
		} else if (buf[l-1] == 's') {
			buf[l-2] = '\0';
			fprintf(fpmk,
"%s.o: ../%s/%s.s\n\t$(CPP) $(INCS) $(DEFS) ../%s/%s.s %s.s\n\
\t$(AS) -o %s.o %s.s\n\n",
				buf, dir, buf, dir, buf, buf, buf, buf);
		} else {
			printf("Unknown file type: %s\n", buf);
			exit(1);
		}

		/*
		 * And tack on another object to our list
		 */
		col += strlen(buf)+3;
		if (objs) {
			int len;

			len = strlen(objs)+strlen(buf)+4;
			if (col > COLS) {
				len += 3;
			}
			objs = realloc(objs, len);
			if (col > COLS) {
				strcat(objs, " \\\n\t");
				col = 8;
			} else {
				strcat(objs, " ");
			}
		} else {
			objs = malloc(strlen(buf)+3);
			objs[0] = '\0';
		}
		strcat(objs, buf);
		strcat(objs, ".o");
		fprintf(fpobj, "%s.o\n", buf);
	}
	fclose(fp);

	/*
	 * Output object list to file
	 */
	fprintf(fpmk, "OBJS=%s\n\n", objs);

	/*
	 * Output make tailer
	 */
	if ((fp = fopen("make.tail", "r")) == NULL) {
		perror("make.tail");
		exit(1);
	}
	while ((c = getc(fp)) != EOF)
		putc(c, fpmk);
	fclose(fp);

	fclose(fpmk);
	fclose(fpobj);
	return(0);
}
@


1.3
log
@Cap length of OBJS line
@
text
@d23 1
a23 1
	if ((fpobj = fopen("objs", "w")) == NULL) {
d31 1
a31 1
	if ((fpmk = fopen("makefile", "w")) == NULL) {
@


1.2
log
@Don't use command.com I/O redirection
@
text
@d7 2
d16 1
a16 1
	int c, l;
d91 1
d93 13
a105 2
			objs = realloc(objs, strlen(objs)+strlen(buf)+4);
			strcat(objs, " ");
@


1.1
log
@Initial revision
@
text
@d78 1
a78 1
"%s.o: ../%s/%s.s\n\t$(CPP) $(INCS) $(DEFS) ../%s/%s.s > %s.s\n\
@
