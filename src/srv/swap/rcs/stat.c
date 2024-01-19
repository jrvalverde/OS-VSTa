head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.4
date	93.11.16.02.48.52;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.23.27.00;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.26.23.29.41;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.12.46;	author vandys;	state Exp;
branches;
next	;


desc
@Stat on swap
@


1.4
log
@Source reorg
@
text
@/*
 * stat.c
 *	Implement stat operations on the swap device
 */
#include <sys/param.h>
#include <sys/swap.h>

extern ulong total_swap, free_swap;

/*
 * swap_stat()
 *	Build stat string for file, send back
 */
void
swap_stat(struct msg *m, struct file *f)
{
	char result[MAXSTAT];

	/*
	 * Root is hard-coded
	 */
	sprintf(result,
	 "perm=1/1\nacc=0/4/2\nsize=%ld\ntype=f\nowner=0\nfree=%ld\n",
		total_swap, free_swap);
	m->m_buf = result;
	m->m_arg = m->m_buflen = strlen(result);
	m->m_nseg = 1;
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}
@


1.3
log
@new UID code
@
text
@a4 1
#include <swap/swap.h>
d6 1
@


1.2
log
@Get rid of extraneous extern
@
text
@d23 1
a23 1
	 "perm=1/1\nacc=0/4/2\nsize=%ld\ntype=f\nowner=1/1\nfree=%ld\n",
@


1.1
log
@Initial revision
@
text
@a7 1
extern char *strerror();
@
