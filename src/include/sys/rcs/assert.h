head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.2
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.2
date	93.08.29.22.55.39;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.13.27;	author vandys;	state Exp;
branches;
next	;


desc
@ASSERT/ASSERT_DEBUG implementation
@


1.2
log
@Add an ANSI prototype for assfail()
@
text
@#ifndef _ASSERT_H
#define _ASSERT_H
/*
 * assert.h
 *	Both debug and everyday assertion interfaces
 */
extern void assfail(const char *, const char *, int);

#define ASSERT(condition, message) \
	if (!(condition)) { assfail(message, __FILE__, __LINE__); }
#ifdef DEBUG
#define ASSERT_DEBUG(c, m) ASSERT(c, m)
#else
#define ASSERT_DEBUG(c, m)
#endif

#endif /* _ASSERT_H */
@


1.1
log
@Initial revision
@
text
@d7 2
@
