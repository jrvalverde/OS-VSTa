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
date	94.10.05.23.26.56;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.04.10.19.47.52;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.16.02.45.04;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.43.38;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.46.58;	author vandys;	state Exp;
branches;
next	;


desc
@Stuff to make fd0/fd1 look like fd/0 and fd/1
@


1.5
log
@Merge in massive fixes for the floppy driver.  It now pretty
much works!
@
text
@/*
 * dir.c
 *	Do readdir() operation
 */
#include <alloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include "fd.h"


#define MAXDIRIO (1024)		/* Max number of bytes in one dir read */


/*
 * add_ent()
 *	Add another entry to the dir buffer if there's room
 *
 * Return 1 if the entry doesn't fit, 0 if it does
 */
static int
add_ent(char *buf, int drv, int sz, uint len)
{
	uint left, x;
	char ent[16];

	if (sz) {
		sprintf(ent, "fd%d_%d", drv, sz / 1024);
	} else {
		sprintf(ent, "fd%d", drv);
	}
	left = len - strlen(buf);
	x = strlen(ent);
	if (left < (x + 2)) {
		return(1);
	}
	strcat(buf, ent);
	strcat(buf, "\n");
	return(0);
}


/*
 * fd_readdir()
 *	Fill in buffer with list of supported names
 */
void
fd_readdir(struct msg *m, struct file *f)
{
	uint len, nent = 0, x, y, entries;
	char *buf;
	struct floppy *flp;

	/*
	 * Get a buffer
	 */
	len = m->m_arg;
	if (len > MAXDIRIO) {
		len = MAXDIRIO;
	}

	buf = (char *)malloc(len);
	if (buf == NULL) {
		msg_err(m->m_sender, ENOMEM);
		return;
	}
	buf[0] = '\0';

	/*
	 * Skip entries until we reach our current position
	 */
	entries = 0;
	for (x = 0; x < NFD; x++) {
		flp = &floppies[x];
		
		/*
		 * Skip drives that aren't present
		 */
		if (flp->f_state == F_NXIO) {
			continue;
		}

		/*
		 * Always have one entry!
		 */
		if (entries >= f->f_pos) {
			nent++;
			if (add_ent(buf, x, 0, len)) {
				goto done;
			}
		}
		entries++;

		/*
		 * Scan the densities table
		 */
		for (y = 0; flp->f_posdens[y] != -1; y++) {
			if (entries >= f->f_pos) {
				int dens = flp->f_posdens[y];
				nent++;
				if (add_ent(buf, x,
					    fdparms[dens].f_size, len)) {
					goto done;
				}
			}
			entries++;
		}
	}

done:
	/*
	 * Send results
	 */
	if (strlen(buf) == 0) {
		/*
		 * If we have nothing in our buffer return EOF
		 */
		m->m_nseg = m->m_arg = 0;
	} else {
		m->m_buf = buf;
		m->m_arg = m->m_buflen = strlen(buf);
		m->m_nseg = 1;
	}
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
	free(buf);
	f->f_pos += nent;
}


/*
 * fd_open()
 *	Move from root dir down into a device
 */
void
fd_open(struct msg *m, struct file *f)
{
	int drv, i;
	char *p = m->m_buf;
	char nm[16];
	struct floppy *fl;


	if (f->f_slot != ROOTDIR) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * All nodes start with our prefix, "fd"
	 */
	if ((strlen(p) < 3) || strncmp(p, "fd", 2)) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * Our next digit is the unit number
	 */
	drv = p[2] - '0';
	if ((drv > NFD) || (floppies[drv].f_state == F_NXIO)) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	fl = &floppies[drv];

	if (!p[3]) {
		/*
		 * We have the special node - return the details
		 */
		f->f_slot = SPECIALNODE;
		f->f_unit = drv;
		fl->f_density = fl->f_specialdens;
		fl->f_opencnt++;

		/*
		 * Pick up user parameters or invalidate the parameter info
		 * to signal that we want to probe the details
		 */
		if (fl->f_specialdens == DISK_USERDEF) {
			fl->f_parms = fl->f_userp;
		} else if (fl->f_lastuseddens == DISK_AUTOPROBE) {
			fl->f_parms.f_size = FD_PUNDEF;
		}
		m->m_arg = m->m_arg1 = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
		return;
	}
	
	for(i = 0; fl->f_posdens[i] != -1; i++) {
		int dens = fl->f_posdens[i];

		sprintf(nm, "_%d", fdparms[dens].f_size / 1024);
		if (!strcmp(&p[3], nm)) {
			/*
			 * We've found our node - return details
			 */
			f->f_slot = dens;
			f->f_unit = drv;
			fl->f_state = F_OFF;
			fl->f_density = dens;
			fl->f_parms = fdparms[dens];
			fl->f_opencnt++;
			m->m_arg = m->m_arg1 = m->m_nseg = 0;
			msg_reply(m->m_sender, m);
			return;
		}
	}

	msg_err(m->m_sender, ESRCH);
}
@


1.4
log
@Cleanup and switch to syslog
@
text
@a10 1
extern struct floppy floppies[];
d12 31
d50 3
a52 12
	int nfd, x, entries;
	char *buf, *p;

	/* 
	 * Count up number of floppy units configured
	 */
	for (x = f->f_pos, nfd = 0; x < NFD; ++x) {
		if (floppies[x].f_state == F_NXIO) {
			continue;
		}
		++nfd;
	}
d55 1
a55 2
	 * Calculate # of entries which will fit.  4 is the chars
	 * in "fdX\n".
d57 3
a59 7
	entries = m->m_arg/4;

	/*
	 * Take smaller of available and fit
	 */
	if (nfd > entries) {
		nfd = entries;
d62 3
a64 6
	/*
	 * End of file--return 0 count
	 */
	if (nfd == 0) {
		m->m_arg = m->m_arg1 = m->m_nseg = 0;
		msg_reply(m->m_sender, m);
d67 1
d70 1
a70 1
	 * Get a temp buffer to build into
d72 21
a92 4
	if ((buf = malloc(entries*4+1)) == 0) {
		msg_err(f->f_sender, ENOMEM);
		return;
	}
d94 13
a106 6
	/*
	 * Assemble entries
	 */
	for (p = buf, x = f->f_pos; (x < NFD) && (nfd > 0); ++x) {
		if (floppies[x].f_state == F_NXIO) {
			continue;
a107 2
		sprintf(p, "fd%d\n", x);
		p += 4;
d110 1
d114 10
a123 3
	m->m_buf = buf;
	m->m_arg = m->m_buflen = p-buf;
	m->m_nseg = 1;
d127 1
a127 1
	f->f_pos += nfd;
d130 1
d138 3
d143 2
a144 1
	if (f->f_unit != ROOTDIR) {
d148 8
a155 2
	if ((strlen(m->m_buf) == 3) && !strncmp(m->m_buf, "fd", 2)) {
		char c = ((char *)(m->m_buf))[2];
d157 51
a207 10
		if ((c >= '0') && (c < ('0'+NFD))) {
			fl = &floppies[c-'0'];
			if (fl->f_state != F_NXIO) {
				f->f_unit = fl->f_unit;
				fl->f_opencnt += 1;
				m->m_buflen = m->m_nseg =
					m->m_arg1 = m->m_arg = 0;
				msg_reply(m->m_sender, m);
				return;
			}
d210 1
@


1.3
log
@Source reorg
@
text
@d5 3
a9 2

extern void *malloc();
@


1.2
log
@Fix directory listing
@
text
@d6 1
a6 1
#include <fd/fd.h>
@


1.1
log
@Initial revision
@
text
@d26 1
a26 1
		if (floppies[x].f_state == F_NXIO)
d28 1
d41 1
a41 1
	if (nfd > entries)
d43 10
d66 1
a66 1
		if (floppies[x].f_state == F_NXIO)
d68 1
d77 1
a77 1
	m->m_buflen = p-buf;
d79 1
a79 1
	m->m_arg = m->m_arg1 = 0;
d82 1
@
