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
date	93.08.13.17.29.52;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.43.36;	author vandys;	state Exp;
branches;
next	;


desc
@Definitions for debugger, especially in-core symbol table
@


1.2
log
@Fix dbsym compile
@
text
@#ifndef _NAMES_H
#define _NAMES_H
/*
 * names.h
 *	Values shared between kernel and utiliies
 */
#define DBG_NAMESZ (20*1024)	/* Buffer for namelist data */

/*
 * Type of each entry in dbg_names[]
 */
#define DBG_END (1)
#define DBG_TEXT (2)
#define DBG_DATA (3)

/*
 * Structure superimposed onto the stream of bytes in dbg_names[]
 */
struct sym {
	unsigned char s_type;		/* Must be first--see dbg_names[] */
	unsigned long s_val;
	char s_name[1];
};

/*
 * How to advance to end of current struct sym
 */
#define NEXTSYM(s) ((struct sym *)((s)->s_name + strlen((s)->s_name) + 1))

#endif /* _NAMES_H */
@


1.1
log
@Initial revision
@
text
@a6 2
#include <sys/types.h>

d20 2
a21 2
	uchar s_type;		/* Must be first--see dbg_names[] */
	ulong s_val;
@
