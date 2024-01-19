head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.5
date	94.06.03.04.47.04;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.02.28.04.59.40;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.45.50;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.28.47;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.43.02;	author vandys;	state Exp;
branches;
next	;


desc
@Stat handling
@


1.5
log
@Remove old proto
@
text
@/*
 * stat.c
 *	Do the stat function
 *
 * We also lump the chmod/chown stuff here as well
 */
#include "cons.h"
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include <std.h>
#include <stdio.h>

extern char *perm_print(struct prot *);

/*
 * cons_stat()
 *	Do stat
 */
void
cons_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];

	if (f->f_screen == ROOTDIR) {
		sprintf(buf, "size=%d\ntype=d\nowner=0\ninode=0\n", NVTY);
	} else {
		struct screen *s = &screens[f->f_screen];

		sprintf(buf,
"size=%d\ntype=c\nowner=0\ninode=%d\nrows=%d\ncols=%d\ngen=%d\n",
			s->s_nbuf, f->f_screen, ROWS, COLS, s->s_gen);
	}
	strcat(buf, perm_print(&cons_prot));
	m->m_buf = buf;
	m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * cons_wstat()
 *	Allow writing of supported stat messages
 */
void
cons_wstat(struct msg *m, struct file *f)
{
	char *field, *val;
	struct screen *s = &screens[f->f_screen];

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, &cons_prot, f->f_flags, &field, &val) == 0)
		return;

	/*
	 * Process each kind of field we can write
	 */
	if (!strcmp(field, "gen")) {
		/*
		 * Set access-generation field
		 */
		if (val) {
			s->s_gen = atoi(val);
		} else {
			s->s_gen += 1;
		}
		f->f_gen = s->s_gen;
	} else if (!strcmp(field, "screen")) {
		select_screen(atoi(val));
	} else {
		/*
		 * Not a field we support...
		 */
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Return success
	 */
	m->m_buflen = m->m_nseg = m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}
@


1.4
log
@Convert stat support for multiple virtual consoles
@
text
@a14 1
extern int do_wstat(struct msg *, struct prot *, uint, char **, char **);
@


1.3
log
@Source reorg
@
text
@d11 2
d14 2
a15 1
extern char *perm_print();
a16 3
extern struct prot cons_prot;
extern int accgen;

d26 9
a34 3
	sprintf(buf,
	 "size=%d\ntype=c\nowner=0\ninode=0\nrows=%d\ncols=%d\ngen=%d\n",
		ROWS*COLS, ROWS, COLS, accgen);
d51 1
d67 1
a67 1
			accgen = atoi(val);
d69 1
a69 1
			accgen += 1;
d71 3
a73 1
		f->f_gen = accgen;
@


1.2
log
@New UID code
@
text
@d7 1
a7 1
#include <cons/cons.h>
@


1.1
log
@Initial revision
@
text
@d27 1
a27 1
	 "size=%d\ntype=c\nowner=1/1\ninode=0\nrows=%d\ncols=%d\ngen=%d\n",
@
