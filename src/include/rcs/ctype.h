head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.5
date	94.09.23.20.38.37;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.19.00.57.26;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.26.18.45.37;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.17.18.23.32;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.48.04;	author vandys;	state Exp;
branches;
next	;


desc
@C types
@


1.5
log
@Add procedural interfaces to all global C library data
@
text
@#ifndef _CTYPE_H
#define _CTYPE_H
/*
 * ctype.h
 *	Categorize character types
 *
 * ctype.c holds the actual tables; these just use bits from that
 * initialized table.  ctype.c also provides functions for each
 * macro; this saves code space at the expense of speed.
 */

extern const unsigned char *__get_ctab(void);

/*
 * Bits in each slot
 */
#define _CT_DIG (1)
#define _CT_HEXDIG (2)
#define _CT_LOWER (4)
#define _CT_UPPER (8)
#define _CT_WHITE (16)
#define _CT_CTRL (32)

/*
 * Macros
 */
#ifndef _CT_NOMACS

/*
 * The table used.  Note that __ctab is queried via __get_ctab()
 * during C startup, and placed here.
 */
extern const unsigned char *__ctab;

#define __bits(c, b) (__ctab[(c) & 0x7F] & (b))
#define isupper(c) __bits((c), _CT_UPPER)
#define islower(c) __bits((c), _CT_LOWER)
#define isalpha(c) __bits((c), _CT_UPPER|_CT_LOWER)
#define isalnum(c) __bits((c), _CT_UPPER|_CT_LOWER|_CT_DIG)
#define isdigit(c) __bits((c), _CT_DIG)
#define isxdigit(c) __bits((c), _CT_HEXDIG)
#define isspace(c) __bits((c), _CT_WHITE)
#define iscntrl(c) __bits((c), _CT_CTRL)
#define ispunct(c) (!iscntrl(c) && !isalnum(c))
#define isprint(c) (!iscntrl(c))
#define isascii(c) (((int)(c) >= 0) && ((int)(c) <= 0x7F))
#define tolower(c) (isupper(c) ? ((c) - 'A' + 'a') : c)
#define toupper(c) (islower(c) ? ((c) - 'a' + 'A') : c)
#define toascii(c) ((c) & 0x7F)
#endif /* !_CT_NOMACS */

#endif /* _CTYPE_H */
@


1.4
log
@Fix precedence mistake in macro
@
text
@d12 1
a12 4
/*
 * The table
 */
extern unsigned char __ctab[];
d28 7
@


1.3
log
@Add toascii()
@
text
@d31 9
a39 9
#define __bits(c, b) (__ctab[c & 0x7F] & b)
#define isupper(c) __bits(c, _CT_UPPER)
#define islower(c) __bits(c, _CT_LOWER)
#define isalpha(c) __bits(c, _CT_UPPER|_CT_LOWER)
#define isalnum(c) __bits(c, _CT_UPPER|_CT_LOWER|_CT_DIG)
#define isdigit(c) __bits(c, _CT_DIG)
#define isxdigit(c) __bits(c, _CT_HEXDIG)
#define isspace(c) __bits(c, _CT_WHITE)
#define iscntrl(c) __bits(c, _CT_CTRL)
@


1.2
log
@Add tolower/toupper
@
text
@d45 1
@


1.1
log
@Initial revision
@
text
@d43 2
@
