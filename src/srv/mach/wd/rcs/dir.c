head	1.6;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.3
	V1_1:1.3
	V1_0:1.2;
locks; strict;
comment	@ * @;


1.6
date	94.10.06.01.56.15;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.03.08.20.06.31;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.03.04.02.02.21;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.45.20;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.02.20.15.52;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.12.19.56.05;	author vandys;	state Exp;
branches;
next	;


desc
@Directory handling
@


1.6
log
@Add support for multiple controllers and such
@
text
@/*
 * dir.c
 *	Do readdir() operation
 */
#include <stdio.h>
#include <sys/fs.h>
#include <std.h>
#include "wd.h"

#define MAXDIRIO (1024)		/* Max # bytes in one dir read */

/*
 * add_ent()
 *	Add another entry to the buffer if there's room
 *
 * Returns 1 if it wouldn't fit, 0 if it fits and was added.
 */
static int
add_ent(char *buf, char *p, uint len)
{
	uint left, x;

	left = len - strlen(buf);
	x = strlen(p);
	if (left < (x+2)) {
		return(1);
	}
	strcat(buf, p);
	strcat(buf, "\n");
	return(0);
}

/*
 * wd_readdir()
 *	Fill in buffer with list of supported names
 */
void
wd_readdir(struct msg *m, struct file *f)
{
	uint x, y, entries, len, nent = 0;
	char *buf;
	struct disk *d;

	/*
	 * Get a buffer
	 */
	len = m->m_arg;
	if (len > MAXDIRIO) {
		len = MAXDIRIO;
	}
	buf = malloc(len);
	if (buf == 0) {
		msg_err(m->m_sender, ENOMEM);
		return;
	}
	buf[0] = '\0';

	/* 
	 * Skip entries until we arrive at the current position
	 */
	entries = 0;
	for (x = 0; x < NWD; ++x) {
		d = &disks[x];
		
		/*
		 * Skip disks not present
		 */
		if (!d->d_configed) {
			continue;
		}

		/*
		 * Scan partition table
		 */
		for (y = 0; y < MAX_PARTS; ++y) {
			if ((d->d_parts[y] == NULL)
					|| (d->d_parts[y]->p_val == 0)) {
				continue;
			}
			if (entries >= f->f_pos) {
				nent++;
				if (add_ent(buf, d->d_parts[y]->p_name, len)) {
					goto done;
				}
			}
			entries++;
		}
	}

done:
	if (strlen(buf) == 0) {
		/*
		 * If nothing was put in our buffer, return a EOF
		 */
		m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
	} else {
		/*
		 * Send results
		 */
		m->m_buf = buf;
		m->m_arg = m->m_buflen = strlen(buf);
		m->m_nseg = 1;
		m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
	}
	free(buf);
	f->f_pos += nent;
}

/*
 * wd_open()
 *	Move from root dir down into a device
 */
void
wd_open(struct msg *m, struct file *f)
{
	uint unit, x;
	char *p = m->m_buf;

	/*
	 * Can only move from root to a node
	 */
	if (f->f_node != ROOTDIR) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * All nodes start with our prefix, "wd"
	 */
	if ((strlen(p) < 3) || strncmp(p, "wd", 2)) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * Next digit is always the unit number
	 */
	unit = p[2] - '0';
	if ((unit > NWD) || (!disks[unit].d_configed)) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * Otherwise scan names for a match
	 */
	for (x = 0; x <= MAX_PARTS; ++x) {
		struct part *part;

		part = disks[unit].d_parts[x];
		if ((part == NULL) || (part->p_val == 0)) {
			continue;
		}
		if (!strcmp(part->p_name, p)) {
			f->f_node = MKNODE(unit, x);
			m->m_nseg = m->m_arg = m->m_arg1 = 0;
			msg_reply(m->m_sender, m);
			return;
		}
	}
	msg_err(m->m_sender, ESRCH);
}
@


1.5
log
@Do partition handling right
@
text
@a11 2
extern struct disk disks[];

d63 2
d68 1
a68 1
		if (!configed[x]) {
d75 1
a75 2
		d = &disks[x];
		for (y = 0; y <= MAX_PARTS; ++y) {
d141 1
a141 1
	if (unit > NWD) {
@


1.4
log
@Convert to -ldpart
@
text
@d150 1
a150 1
	for (x = FIRST_PART; x <= LAST_PART; ++x) {
@


1.3
log
@Source reorg
@
text
@d5 1
d20 1
a20 1
static
d43 1
a43 1
	char *buf, *p, tmp[32];
a72 12
		 * Whole disk counts as an entry
		 */
		if (entries >= f->f_pos) {
			sprintf(tmp, "wd%d", x);
			nent++;
			if (add_ent(buf, tmp, len)) {
				goto done;
			}
		}
		entries++;

		/*
d76 3
a78 2
		for (y = 0; y < NPART; ++y) {
			if (d->d_parts[y].p_val == 0) {
d83 1
a83 1
				if (add_ent(buf, d->d_parts[y].p_name, len)) {
d139 1
a139 1
	 * Next digit is always unit #
a147 11
	 * If its's just "wdN", it's the whole-disk interface
	 * XXX add per-partition protection
	 */
	if (strlen(p) == 3) {
		f->f_node = MKNODE(unit, WHOLE_DISK);
		m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return;
	}

	/*
d150 1
a150 1
	for (x = 0; x < NPART; ++x) {
d153 2
a154 2
		part = &disks[unit].d_parts[x];
		if (part->p_val == 0) {
@


1.2
log
@configed[] defined in .h now
@
text
@a5 1
#include <wd/wd.h>
d7 1
@


1.1
log
@Initial revision
@
text
@a11 1
extern int configed[];
@
