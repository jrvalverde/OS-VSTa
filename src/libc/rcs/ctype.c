head	1.8;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.7
	V1_1:1.6
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.8
date	94.09.23.20.37.41;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.12.09.06.18.58;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.10.01.19.07.33;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.19.00.57.01;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.26.18.44.04;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.23.18.18.31;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.17.18.23.23;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.58.51;	author vandys;	state Exp;
branches;
next	;


desc
@Underlying tables for the <ctype.h> interface
@


1.8
log
@Create procedural interfaces to all global C library data
@
text
@/*
 * ctype.c
 *	Master table as well as function versions of macros
 */
#define _CT_NOMACS
#include <ctype.h>

/*
 * Hand-generated table of bits from ctype.h.  I would have used
 * symbolics but then the table is a LOT harder to get into this
 * neat columnar format.
 */
static const unsigned char __ctab[] = {
32,	32,	32,	32,	32,	32,	32,	32,	/* 000 */
32,	48,	48,	48,	48,	48,	32,	32,	/* 008 */
32,	32,	32,	32,	32,	32,	32,	32,	/* 016 */
32,	32,	32,	32,	32,	32,	32,	32,	/* 024 */
16,	0,	0,	0,	0,	0,	0,	0,	/* 032 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 040 */
3,	3,	3,	3,	3,	3,	3,	3,	/* 048 */
3,	3,	0,	0,	0,	0,	0,	0,	/* 056 */
0,	10,	10,	10,	10,	10,	10,	8,	/* 064 */
8,	8,	8,	8,	8,	8,	8,	8,	/* 072 */
8,	8,	8,	8,	8,	8,	8,	8,	/* 080 */
8,	8,	8,	0,	0,	0,	0,	0,	/* 088 */
0,	6,	6,	6,	6,	6,	6,	4,	/* 096 */
4,	4,	4,	4,	4,	4,	4,	4,	/* 104 */
4,	4,	4,	4,	4,	4,	4,	4,	/* 112 */
4,	4,	4,	0,	0,	0,	0,	0	/* 120 */
};

const unsigned char *
__get_ctab(void)
{
	return(__ctab);
}

#define __bits(c, b) (__ctab[(c) & 0x7F] & (b))

isupper(int c) {
	return __bits(c, _CT_UPPER);
}
islower(int c) {
	return __bits(c, _CT_LOWER);
}
isalpha(int c) {
	return __bits(c, _CT_UPPER|_CT_LOWER);
}
isalnum(int c) {
	return __bits(c, _CT_UPPER|_CT_LOWER|_CT_DIG);
}
isdigit(int c) {
	return __bits(c, _CT_DIG);
}
isxdigit(int c) {
	return __bits(c, _CT_HEXDIG);
}
isspace(int c) {
	return __bits(c, _CT_WHITE);
}
iscntrl(int c) {
	return __bits(c, _CT_CTRL);
}
ispunct(int c) {
	return (!iscntrl(c) && !isalnum(c));
}
isprint(int c) {
	return (!iscntrl(c));
}
isascii(int c) {
	return (((int)(c) >= 0) && ((int)(c) <= 0x7F));
}
tolower(int c) {
	if (isupper(c)) {
		return(c - 'A' + 'a');
	}
	return(c);
}
toupper(int c) {
	if (islower(c)) {
		return(c - 'a' + 'A');
	}
	return(c);
}
toascii(int c) {
	return(c & 0x7F);
}
@


1.7
log
@Forgot digits are isxdigit() also
@
text
@d13 1
a13 1
unsigned char __ctab[] = {
d31 6
@


1.6
log
@Fix what isspace()
@
text
@d20 2
a21 2
1,	1,	1,	1,	1,	1,	1,	1,	/* 048 */
1,	1,	0,	0,	0,	0,	0,	0,	/* 056 */
@


1.5
log
@Boo-boo in operator precedence in a macro
@
text
@d15 1
a15 1
48,	48,	32,	48,	48,	32,	32,	32,	/* 008 */
@


1.4
log
@Add toascii()
@
text
@d32 1
a32 1
#define __bits(c, b) (__ctab[c & 0x7F] & b)
@


1.3
log
@a/A are at 65/96, so my whole table was off by one
@
text
@d79 3
@


1.2
log
@Add tolower/toupper
@
text
@d22 1
a22 1
10,	10,	10,	10,	10,	10,	8,	8,	/* 064 */
d25 2
a26 2
8,	8,	0,	0,	0,	0,	0,	0,	/* 088 */
6,	6,	6,	6,	6,	6,	4,	4,	/* 096 */
d29 1
a29 1
4,	4,	0,	0,	0,	0,	0,	0	/* 120 */
@


1.1
log
@Initial revision
@
text
@d67 12
@
