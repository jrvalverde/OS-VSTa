head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.2
date	94.04.11.00.35.04;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.09.00.26.27;	author vandys;	state Exp;
branches;
next	;


desc
@Code for handling map objects
@


1.2
log
@Fix warning for volatile treatment
@
text
@/*
 * map.c
 *	Map data structure manipulation
 */
#include "map.h"
#include <std.h>

/*
 * alloc_map()
 *	Get a map
 */
struct map *
alloc_map(void)
{
	struct map *m;

	m = malloc(sizeof(struct map));
	if (m) {
		m->m_nmap = 0;
	}
	return(m);
}

/*
 * add_map()
 *	Add another mapping
 *
 * Returns 1 if it can't fit, 0 on success
 */
int
add_map(struct map *m, void *base, ulong len, ulong off)
{
	struct mslot *ms;

	if (m->m_nmap >= NMAP) {
		return(1);
	}
	ms = &m->m_map[m->m_nmap++];
	ms->m_addr = base;
	ms->m_len = len;
	ms->m_off = off;
	return(0);
}

/*
 * do_map()
 *	Given vaddr, return offset
 *
 * Offset is passed in *off; return 1 on illegal addr, 0 on success
 */
int
do_map(const struct map *m, const void *vaddr, ulong *off)
{
	uint x;

	for (x = 0; x < m->m_nmap; ++x) {
		struct mslot *ms;

		ms = (struct mslot *)&m->m_map[x];
		if (vaddr < ms->m_addr) {
			continue;
		}
		if ((char *)vaddr >= ((char *)ms->m_addr + ms->m_len)) {
			continue;
		}
		*off = ((char *)vaddr - (char *)ms->m_addr) + ms->m_off;
		return(0);
	}
	return(1);
}
@


1.1
log
@Initial revision
@
text
@d59 1
a59 1
		ms = &m->m_map[x];
@
