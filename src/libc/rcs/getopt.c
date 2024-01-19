head	1.3;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.3
date	94.09.26.17.14.12;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.09.23.20.37.41;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.26.18.43.48;	author vandys;	state Exp;
branches;
next	;


desc
@getopt() from UCB distribution
@


1.3
log
@Hide macros so we can use original source
@
text
@/*
 * getopt.c
 *
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>

/*
 * get option letter from argument vector
 */
#undef opterr
#undef optind
#undef optarg
static
int	opterr = 1,		/* if error message should be printed */
	optind = 1,		/* index into parent argv vector */
	optopt;			/* character checked for validity */
static
char	*optarg;		/* argument associated with option */

#define	BADCH	(int)'?'
#define	EMSG	""

void *
__getopt_ptr(int idx)
{
	switch (idx) {
	case 0: return(&opterr);
	case 1: return(&optind);
	case 2: return(&optopt);
	case 3: return(&optarg);
	default: return(0);
	}
}

int
getopt(nargc, nargv, ostr)
	int nargc;
	char **nargv;
	char *ostr;
{
	static char *place = EMSG;		/* option letter processing */
	register char *oli;			/* option letter list index */
	char *p;

	if (!*place) {				/* update scanning pointer */
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return(EOF);
		}
		if (place[1] && *++place == '-') {	/* found "--" */
			++optind;
			place = EMSG;
			return(EOF);
		}
	}					/* option letter okay? */
	if ((optopt = (int)*place++) == (int)':' ||
	    !(oli = index(ostr, optopt))) {
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means EOF.
		 */
		if (optopt == (int)'-')
			return(EOF);
		if (!*place)
			++optind;
		if (opterr) {
			if (!(p = rindex(*nargv, '/')))
				p = *nargv;
			else
				++p;
			(void)fprintf(stderr, "%s: illegal option -- %c\n",
			    p, optopt);
		}
		return(BADCH);
	}
	if (*++oli != ':') {			/* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else {					/* need an argument */
		if (*place)			/* no white space */
			optarg = place;
		else if (nargc <= ++optind) {	/* no arg */
			place = EMSG;
			if (!(p = rindex(*nargv, '/')))
				p = *nargv;
			else
				++p;
			if (opterr)
				(void)fprintf(stderr,
				    "%s: option requires an argument -- %c\n",
				    p, optopt);
			return(BADCH);
		}
	 	else				/* white space */
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return(optopt);				/* dump back option letter */
}
@


1.2
log
@Create procedural interfaces to all global C library data
@
text
@d13 3
@


1.1
log
@Initial revision
@
text
@d13 1
d17 1
d22 12
@
