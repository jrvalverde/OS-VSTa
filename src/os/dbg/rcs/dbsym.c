head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	94.04.06.03.36.28;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.03.08.23.22.25;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.13.17.29.52;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.23.22.40.42;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.44.03;	author vandys;	state Exp;
branches;
next	;


desc
@Debugger symbol table handling
@


1.5
log
@Use our own aout.h
@
text
@/* Written by Pace Willisson (pace@@blitz.com)
 * and placed in the public domain.
 * Hacked for VSTa by Andy Valencia (jtk@@netcom.com).  This file is
 * still in the public domain.
 */
#ifdef KDB
#include <stdio.h>
#include "../../include/mach/aout.h"
#include "../../include/mach/nlist.h"
#include "../dbg/dbg.h"

extern void *malloc();

#define FILE_OFFSET(vadr) \
	(vadr - N_DATADDR(hdr) + N_DATOFF(hdr))

struct nlist *old_syms;		/* a.out information */
int num_old_syms;
char *old_strtab;
int old_strtab_size;

struct sym *newsyms;		/* information in kernel format */
struct sym *newptr;		/* Where to add next element */
int newsize = 0;		/* # bytes in newsyms now */

int db_symtabsize_adr;
int db_symtab_adr;

int avail;

usage()
{
	fprintf(stderr, "usage: dbsym file\n");
	exit(1);
}

struct aout hdr;

main (argc, argv)
	int argc;
	char **argv;
{
	FILE *f;
	char *name, *buf, *p;
	int c, i, len;
	struct nlist *sp;


	/*
	 * Single argument--kernel to patch
	 */
	if (argc != 2) {
		usage();
	}
	name = argv[1];

	/*
	 * Get storage for symbols
	 */
	newsyms = malloc(DBG_NAMESZ);
	if (!newsyms) {
		perror("malloc");
		exit(1);
	}
	newptr = newsyms;

	if ((f = fopen(name, "r+b")) == NULL) {
		fprintf(stderr, "can't open %s\n", name);
		exit(1);
	}

	if (fread((char *)&hdr, sizeof hdr, 1, f) != 1) {
		fprintf(stderr, "can't read header\n");
		exit(1);
	}

	if (N_BADMAG(hdr)) {
		fprintf(stderr, "bad magic number\n");
		exit(1);
	}

	if (hdr.a_syms == 0) {
		fprintf(stderr, "no symbols\n");
		exit(1);
	}

	fseek (f, N_STROFF(hdr), 0);
	if (fread((char *)&old_strtab_size, sizeof(int), 1, f) != 1) {
		fprintf(stderr, "can't read old strtab size\n");
		exit(1);
	}

	if ((old_syms = (struct nlist *)malloc(hdr.a_syms)) == NULL
	    || ((old_strtab = malloc(old_strtab_size)) == NULL)) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	fseek(f, N_SYMOFF(hdr), 0);
	if (fread((char *)old_syms, hdr.a_syms, 1, f) != 1) {
		fprintf (stderr, "can't read symbols\n");
		exit (1);
	}

	fseek(f, N_STROFF(hdr), 0);
	if (fread((char *)old_strtab, old_strtab_size, 1, f) != 1) {
		fprintf (stderr, "can't read string table\n");
		exit (1);
	}

	num_old_syms = hdr.a_syms / sizeof (struct nlist);

	for (i = 0, sp = old_syms; i < num_old_syms; i++, sp++) {
		if (sp->n_type & N_STAB)
			continue;
		if (sp->n_un.n_strx == 0)
			continue;

		if (sp->n_value < 0x1000)
			continue;

		if (sp->n_value >= 0x10000000)
			continue;

		name = old_strtab + sp->n_un.n_strx;

		len = strlen(name);
		if (len == 0)
			continue;

		/*
		 * These are cruft
		 */
		if (strchr(name, '.')) {
			continue;
		}

		/*
		 * Ignore leading '_'s
		 */
		if (name[0] == '_') {
			++name;
		}

		/*
		 * There are 256 of them, they all fall into a common
		 * point, so save all that symbol space.
		 */
		if (!strncmp(name, "xint", 4)) {
			continue;
		}

		/*
		 * Ignore GNU C stuff
		 */
		if (!strncmp(name, "__gnu_compiled", 14)) {
			continue;
		}

		/*
		 * Create room for new element, add to newsyms
		 */
		newsize += sizeof(struct sym) + strlen(name);
		if (newsize >= DBG_NAMESZ) {
			fprintf(stderr, "Too many symbols\n");
			exit(1);
		}
		newptr->s_type = DBG_TEXT;	/* XXX DBG_DATA */
		newptr->s_val = sp->n_value;
		strcpy(newptr->s_name, name);
		newptr = NEXTSYM(newptr);

		/*
		 * Record these when we see them go by
		 */
		if (strcmp(name, "dbg_names") == 0) {
			db_symtab_adr = sp->n_value;
		}
		if (strcmp(name, "dbg_names_len") == 0) {
			db_symtabsize_adr = sp->n_value;
		}
	}

	/*
	 * Add a sentinel at the end
	 */
	newsize += sizeof(struct sym);
	if (newsize >= DBG_NAMESZ) {
		fprintf(stderr, "Too many symbols\n");
		exit(1);
	}
	newptr->s_type = DBG_END;

	/*
	 * Make sure these two required symbols were spotted
	 */
	if (db_symtab_adr == 0 || db_symtabsize_adr == 0) {
		fprintf(stderr, "couldn't find dbg_names symbols\n");
		exit(1);
	}

	/*
	 * We now know how much room we need.  Verify that the
	 * kernel has enough room for us to write it in.
	 */
	fseek(f, FILE_OFFSET(db_symtabsize_adr), 0);
	if (fread((char *)&avail, sizeof (int), 1, f) != 1) {
		fprintf (stderr, "can't read symtabsize\n");
		exit (1);
	}
	printf ("dbsym: need %d; avail %d\n", newsize, avail);
	if (newsize > avail) {
		fprintf (stderr, "not enough room in dbg_names array\n");
		exit (1);
	}

	/*
	 * Yup.  Seek to initialized data and put it in place
	 */
	fseek(f, FILE_OFFSET(db_symtab_adr), 0);
	i = fwrite(newsyms, sizeof(char), newsize, f);
	if (i != newsize) {
		if (i < 0) {
			perror("fwrite");
		}
		fprintf(stderr, "Write of syms failed, code %d\n", i);
		exit(1);
	}
	fclose(f);
	return(0);
}

#else /* !KDB */

/*
 * When not debugging, no need to stuff symbols into kernel
 */
main()
{
	return(0);
}

#endif /* KDB */
@


1.4
log
@Ignore GNU C phlegm
@
text
@d8 2
a9 1
#include <aout.h>
d37 1
a37 1
struct exec hdr;
@


1.3
log
@Fix dbsym compile
@
text
@d153 7
@


1.2
log
@Implement KDB
@
text
@d9 1
a9 1
#include <dbg/dbg.h>
@


1.1
log
@Initial revision
@
text
@d6 1
a6 1
#ifdef DEBUG
d225 1
a225 1
#else /* !DEBUG */
d235 1
a235 1
#endif
@
