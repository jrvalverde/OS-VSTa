head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.2
	V1_1:1.2
	V1_0:1.1;
locks; strict;
comment	@ * @;


1.5
date	94.10.06.01.56.15;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.10.20.14.26;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.03.04.02.02.21;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.11.16.02.45.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.12.19.57.08;	author vandys;	state Exp;
branches;
next	;


desc
@Handling of stat/wstat
@


1.5
log
@Add support for multiple controllers and such
@
text
@/*
 * stat.c
 *	Do the stat function
 */
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include <std.h>
#include <stdio.h>
#include "wd.h"

extern char *perm_print();

/*
 * find_prot()
 *	Given node, return pointer to right prot structure
 */
static struct prot *
find_prot(int node)
{
	struct prot *p;
	uint unit, part;

	if (node == ROOTDIR) {
		p = &wd_prot;
	} else {
		unit = NODE_UNIT(node);
		part = NODE_SLOT(node);
		p = &disks[unit].d_parts[part]->p_prot;
	}
	return(p);
}

/*
 * wd_stat()
 *	Do stat
 */
void
wd_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];
	uint size, pextoffs, node;
	char type;
	struct prot *p;

	if (f->f_node == ROOTDIR) {
		int x;

		size = 0;
		node = 0;
		type = 'd';
		pextoffs = 0;
		
		/*
		 * Work out the dir size
		 */
		for (x = 0; x < NWD; x++) {
			if (disks[x].d_configed) {
				int y;
				
				for (y = 0; y < MAX_PARTS; y++) {
					if (disks[x].d_parts[y]
					    && disks[x].d_parts[y]->p_val) {
						size++;
					}
				}
			}
		}
	} else {
		uint part, unit;

		node = f->f_node + UNITSTEP;
		unit = NODE_UNIT(f->f_node);
		part = NODE_SLOT(f->f_node);
		size = disks[unit].d_parts[part]->p_len;
		size *= SECSZ;
		type = 's';
		pextoffs = disks[unit].d_parts[part]->p_extoffs;
	}
	p = find_prot(f->f_node);
	sprintf(buf,
		"size=%d\ntype=%c\nowner=1/1\ninode=%d\ndev=%d\n" \
		"pextoffs=%d\nirq=%d\nbaseio=0x%x\n",
		size, type, node, wdname, pextoffs, wd_irq, wd_baseio);
	strcat(buf, perm_print(p));
	m->m_buf = buf;
	m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * wd_wstat()
 *	Allow writing of supported stat messages
 */
void
wd_wstat(struct msg *m, struct file *f)
{
	char *field, *val;
	struct prot *p;

	/*
	 * Get pointer to right protection structure
	 */
	p = find_prot(f->f_node);

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, p, f->f_flags, &field, &val) == 0) {
		return;
	}

	/*
	 * Otherwise forget it
	 */
	msg_err(m->m_sender, EINVAL);
}
@


1.4
log
@Add support for partition offset information
@
text
@a12 4
extern struct wdparms parm[];
extern struct disk disks[];
extern struct prot wd_prot;
extern int do_wstat();
d29 1
a29 5
		if (part == WHOLE_DISK) {
			p = &disks[unit].d_prot;
		} else {
			p = &disks[unit].d_parts[part]->p_prot;
		}
d47 3
a49 1
		size = NWD;
d53 16
d72 4
a75 8
		node = f->f_node;
		unit = NODE_UNIT(node);
		part = NODE_SLOT(node);
		if (part == WHOLE_DISK) {
			size = parm[unit].w_size;
		} else {
			size = disks[unit].d_parts[part]->p_len;
		}
d82 3
a84 2
		"size=%d\ntype=%c\nowner=1/1\ninode=%d\npextoffs=%d\n",
		size, type, node, pextoffs);
@


1.3
log
@Convert to -ldpart
@
text
@d50 1
a50 1
	uint size, node;
d58 1
d72 1
d76 2
a77 1
	 "size=%d\ntype=%c\nowner=1/1\ninode=%d\n", size, type, node);
@


1.2
log
@Source reorg
@
text
@d8 2
d16 1
d36 1
a36 1
			p = &disks[unit].d_parts[part].p_prot;
d50 1
a50 1
	uint size, node, part1;
d67 1
a67 1
			size = disks[unit].d_parts[part].p_len;
@


1.1
log
@Initial revision
@
text
@a4 1
#include <wd/wd.h>
d8 1
@
