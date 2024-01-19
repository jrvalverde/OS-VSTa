head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.2
date	93.06.30.02.27.53;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.40.52;	author vandys;	state Exp;
branches;
next	;


desc
@Load a.out's into memory
@


1.2
log
@Make a.out header available to running kernel at its usual address
@
text
@/*
 * load.c
 *	Routines for loading memory from files
 */
#include <stdlib.h>
#include "boot.h"

extern ulong pbase;

/*
 * load_kernel()
 *	Load kernel from its a.out image
 */
void
load_kernel(struct aouthdr *h, FILE *fp)
{
	/*
	 * The first page is invalid to catch null pointers
	 */
	pbase += NBPG;

	/*
	 * Read in the a.out header and text section
	 */
	(void) rewind(fp);
	lread(fp, pbase, sizeof(struct aouthdr));
	pbase += sizeof(struct aouthdr);
	lread(fp, pbase, h->a_text);
	pbase += h->a_text;
	printf(" %ld", h->a_text); fflush(stdout);

	/*
	 * Read in data
	 */
	lread(fp, pbase, h->a_data);
	pbase += h->a_data;
	printf("+%ld", h->a_data); fflush(stdout);

	/*
	 * Zero out BSS
	 */
	lbzero(pbase, h->a_bss);
	pbase += h->a_bss;
	printf("+%ld\n", h->a_bss);
}

/*
 * load_image()
 *	Load a boot task image into the data area
 *
 * This is different than the kernel task load; these images are
 * not in runnable format.  They're simply copied end-to-end after
 * the _end location of the kernel task.
 */
void
load_image(FILE *fp)
{
	struct aouthdr *h;

	/*
	 * Get header
	 */
	h = lptr(pbase);
	lread(fp, pbase, sizeof(struct aouthdr));
	pbase += sizeof(struct aouthdr);
	lread(fp, pbase, h->a_text);
	pbase += h->a_text;
	lread(fp, pbase, h->a_data);
	pbase += h->a_data;
	lbzero(pbase, h->a_bss);
	pbase += h->a_bss;
}
@


1.1
log
@Initial revision
@
text
@d23 1
a23 1
	 * Read in the text section
d25 2
a26 1
	(void)fseek(fp, sizeof(struct aouthdr), SEEK_SET);
@
