head	1.17;
access;
symbols
	V1_3_1:1.10
	V1_3:1.10
	V1_2:1.8
	V1_1:1.8
	V1_0:1.5;
locks; strict;
comment	@ * @;


1.17
date	94.11.15.05.23.06;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.10.23.17.42.37;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.09.23.20.36.37;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.07.10.18.50.02;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.06.21.20.58.35;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.06.07.23.02.10;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.05.30.21.28.28;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.03.23.21.57.24;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.02.28.22.04.33;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.48.09;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.10.06.23.23.05;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.10.01.22.23.59;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.06.30.19.57.42;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.20.21.26.36;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.19.21.42.53;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.12.23.30.14;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Stat messages
@


1.17
log
@Pass node, not file.  dir_timestamp only needs node-level info.
@
text
@/*
 * stat.c
 *	Implement stat operations on an open file
 *
 * The date/time handling functions are derived from software
 * distributed in the Mach and BSD 4.4 software releases.  They
 * were originally written by Bill Jolitz.
 */
#include <sys/fs.h>
#include <sys/perm.h>
#include "dos.h"
#include <sys/param.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <std.h>

extern struct prot dos_prot;

/*
 * ytos()
 *	convert years to seconds (from 1990)
 */
static ulong
ytos(uint y)
{
	uint i;
	ulong ret;

	if (y < 1990) {
		syslog(LOG_WARNING, "year %d is less than 1990!\n", y);
	}
	ret = 0;
	for (i = 1990; i < y; i++) {
		if (i % 4) {
			ret += 365*24*60*60;
		} else {
			ret += 366*24*60*60;
		}
	}
	return(ret);
}

/*
 * mtos()
 *	convert months to seconds
 */
static ulong
mtos(uint m, int leap)
{
	uint i;
	ulong ret;

	ret = 0;
	for (i = 1; i < m; i++) {
		switch(i){
		case 1: case 3: case 5: case 7: case 8: case 10: case 12:
			ret += 31*24*60*60; break;
		case 4: case 6: case 9: case 11:
			ret += 30*24*60*60; break;
		case 2:
			if (leap) ret += 29*24*60*60;
			else ret += 28*24*60*60;
		}
	}
	return ret;
}


/*
 * cvt_time()
 *	Calculate time in seconds given DOS-encoded date/time
 *
 * Returns seconds since 1990, or 0L on failure.
 */
ulong
cvt_time(uint date, uint time)
{
	ulong sec;
	uint leap, t, yd;

	sec = (date >> 9) + 1980;
	leap = !(sec % 4); sec = ytos(sec);			/* year */
	yd = mtos((date >> 5) & 0xF,leap); sec += yd;		/* month */
	t = ((date & 0x1F)-1) * 24*60*60; sec += t;
		yd += t;					/* date */
	sec += (time >> 11) * 60*60;				/* hour */
	sec += ((time >> 5) & 0x3F) * 60;			/* minutes */
	sec += ((time & 0x1F) << 1);				/* seconds */

	return(sec);
}

/*
 * inum()
 *	Synthesize an "inode" number for the node
 */
uint
inum(struct node *n)
{
	extern struct node *rootdir;

	/*
	 * Root dir--no cluster, just give a value of 0
	 */
	if (n == rootdir) {
		return(0);
	}

	/*
	 * Dir--use value of first cluster
	 */
	if (n->n_type == T_DIR) {
		return(n->n_clust->c_clust[0]);
	}

	/*
	 * File in root dir--again, no cluster for root dir.  Just
	 * use cluster value 1 (they start at 2, so this is available),
	 * and or in our slot.
	 */
	if (n->n_dir == rootdir) {
		return ((1 << 16) | n->n_slot);
	} else {
		/*
		 * Others--high 16 bits is cluster of file's directory,
		 * low 16 bits is our slot number.
		 */
		return ((n->n_dir->n_clust->c_clust[0] << 16) | n->n_slot);
	}
}

/*
 * isize()
 *	Calculate size of file from its "inode" information
 */
static uint
isize(struct node *n)
{
	extern uint dirents;

	if (n == rootdir) {
		return(sizeof(struct directory)*dirents);
	}
	return(n->n_clust->c_nclust*clsize);
}

/*
 * dos_acc()
 *	Give access string based on dir entry protection attribute
 */
static char *
dos_acc(struct directory *d)
{
	if (d->attr & DA_READONLY) {
		return("5/0/0");
	}
	return("5/0/2");
}

/*
 * dos_stat()
 *	Build stat string for file, send back
 */
void
dos_stat(struct msg *m, struct file *f)
{
	char result[MAXSTAT];
	struct node *n = f->f_node;
	struct directory d;

	/*
	 * Directories
	 */
	if (n != rootdir) {
		dir_copy(n->n_dir, n->n_slot, &d);
	} else {
		bzero(&d, sizeof(d));
	}
	if (n->n_type == T_DIR) {
		sprintf(result,
 "perm=1/1\nacc=%s\nsize=%d\ntype=d\nowner=0\ninode=%d\nmtime=%ld\n",
			dos_acc(&d), isize(n), inum(n),
			cvt_time(d.date, d.time));
	} else {
		/*
		 * Otherwise look up file and get dope
		 */
		sprintf(result,
 "perm=1/1\nacc=%s\nsize=%d\ntype=f\nowner=0\ninode=%d\nmtime=%ld\n",
			dos_acc(&d), n->n_len, inum(n),
			cvt_time(d.date, d.time));
	}
	m->m_buf = result;
	m->m_buflen = strlen(result);
	m->m_nseg = 1;
	m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * dos_fid()
 *	Return ID for file
 */
void
dos_fid(struct msg *m, struct file *f)
{
	struct node *n = f->f_node;

	/*
	 * Only *files* get an ID (and thus can be mapped shared)
	 */
	if (n->n_type == T_DIR) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * arg is the inode value; arg1 is the size in pages
	 */
	m->m_arg = inum(n);
	m->m_arg1 = btorp(isize(n));
	m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * accum_ro()
 *	Walk a protection label and see if it's writable
 */
static int
accum_ro(struct prot *p)
{
	uint flags, x;

	flags = p->prot_default;
	for (x = 0; x < p->prot_len; ++x) {
		flags |= p->prot_bits[x];
	}
	return((flags & ACC_WRITE) == 0);
}

/*
 * change_mode()
 *	Given before->after chmod, apply to DOS dir entry of node
 */
static void
change_mode(struct prot *old, struct prot *new, struct file *f)
{
	int old_ro, new_ro;

	old_ro = accum_ro(old);
	new_ro = accum_ro(new);
	if (old_ro != new_ro) {
		dir_readonly(f, new_ro);
	}
}

/*
 * dos_wstat()
 *	Write status of DOS file
 *
 * We support setting of modification time only, plus the usual
 * shared code to set access to the filesystem.
 */
void
dos_wstat(struct msg *m, struct file *f)
{
	char *field, *val;
	struct prot *prot, tmp_prot;

	/*
	 * Use the root protection node for root dir, a private
	 * copy otherwise.
	 */
	if (f->f_node == rootdir) {
		prot = &dos_prot;
	} else {
		prot = &tmp_prot;
		tmp_prot = dos_prot;
	}

	/*
	 * Common wstat handling code
	 */
	if (do_wstat(m, prot, f->f_perm, &field, &val) == 0) {
		/*
		 * If he changed the protection, map it back onto
		 * the DOS dir entry.
		 */
		if ((prot == &tmp_prot)  &&
				bcmp(prot, &dos_prot, sizeof(*prot))) {
			change_mode(&dos_prot, prot, f);
		}
		return;
	}
	if (!strcmp(field, "atime") || !strcmp(field, "mtime")) {
		time_t t;

		/*
		 * Convert to number, write to dir entry
		 */
		t = atoi(val);
		dir_timestamp(f->f_node, t);
	} else if (!strcmp(field, "type")) {
		if (dir_set_type(f, val)) {
			msg_err(m->m_sender, EINVAL);
			return;
		}
	} else {
		/*
		 * Unsupported operation
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


1.16
log
@Implement read-only attribute.  Also fix flushing of
symlink type to disk.
@
text
@d304 1
a304 1
		dir_timestamp(f, t);
@


1.15
log
@Add symlink support
@
text
@d10 1
d149 13
d182 2
a183 2
 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=d\nowner=0\ninode=%d\nmtime=%ld\n",
			isize(n), inum(n),
d190 2
a191 2
 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=f\nowner=0\ninode=%d\nmtime=%ld\n",
			n->n_len, inum(n),
d228 32
d270 12
d286 9
a294 1
	if (do_wstat(m, &dos_prot, f->f_perm, &field, &val) == 0) {
@


1.14
log
@Add setting of mtime for DOS files
@
text
@d208 1
a208 1
	m->m_arg1 = btop(isize(n));
d239 5
@


1.13
log
@Convert to openlog()
@
text
@d15 1
d17 2
a79 4
#ifdef XXX
	const uint dayst = 119,		/* Daylight savings, sort of */
		dayen = 303;
#endif
a89 7
#ifdef XXX
	/* Convert to daylight saving */
	yd = yd / (24*60*60);
	if ((yd >= dayst) && ( yd <= dayen)) {
		sec -= 60*60;
	}
#endif
d210 41
@


1.12
log
@Don't apply daylight savings adjustment
@
text
@a15 2
extern char dos_sysmsg[];	/* String used as a prefix in syslog calls */

d27 1
a27 2
		syslog(LOG_WARNING, "%s year %d is less than 1990!\n",
			dos_sysmsg, y);
@


1.11
log
@Syslog and time support
@
text
@d80 1
d83 1
d94 1
d100 1
@


1.10
log
@Fix -Wall warnings
@
text
@d16 2
d29 2
a30 1
		syslog(LOG_WARNING, "year %d is less than 1990!\n", y);
@


1.9
log
@Convert to syslog()
@
text
@a10 1
#include <std.h>
d13 2
a76 1
	uint sa, s;
@


1.8
log
@Source reorg
@
text
@d13 1
d26 1
a26 1
		printf("Warning: year %d is less than 1990!\n", y);
@


1.7
log
@Special case for rootdir, gag.
@
text
@d10 1
a10 1
#include <dos/dos.h>
@


1.6
log
@Add mtime stat field
@
text
@d164 5
a168 1
	dir_copy(n->n_dir, n->n_slot, &d);
@


1.5
log
@Fix struct alignment, can't assume bitfields will fall on byte
boundary any more.
@
text
@d4 4
d15 82
d159 1
d164 1
d167 3
a169 2
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=d\nowner=0\ninode=%d\n",
			isize(n), inum(n));
d175 3
a177 2
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=f\nowner=0\ninode=%d\n",
			n->n_len, inum(n));
@


1.4
log
@Make inum() global (_remove uses it now)
@
text
@d56 1
a56 1
	extern struct boot bootb;
d59 1
a59 1
		return(sizeof(struct directory)*bootb.dirents);
@


1.3
log
@Add FID support
@
text
@d14 1
a14 1
static uint
@


1.2
log
@New UID code
@
text
@d8 1
d93 26
@


1.1
log
@Initial revision
@
text
@d78 1
a78 1
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=d\nowner=1/1\ninode=%d\n",
d85 1
a85 1
		 "perm=1/1\nacc=5/0/2\nsize=%d\ntype=f\nowner=1/1\ninode=%d\n",
@
