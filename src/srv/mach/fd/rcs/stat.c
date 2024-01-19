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
date	93.04.12.23.29.08;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.47.52;	author vandys;	state Exp;
branches;
next	;


desc
@Stat functions
@


1.5
log
@Merge in massive fixes for the floppy driver.  It now pretty
much works!
@
text
@/*
 * stat.c
 *	Do the stat function
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/perm.h>
#include <sys/fs.h>
#include "fd.h"


extern char *perm_print();
extern struct floppy *unit();


extern int fdc_type;
extern int fd_baseio;
extern int fd_dma;
extern int fd_irq;
extern int fd_retries;
extern int fd_messages;
extern port_name fdport_name;
extern struct prot fd_prot;
extern char fdc_names[][FDC_NAMEMAX];

static char fdm_opts[][9] = {"all", "warning", "fail", "critical"};


/*
 * fd_stat()
 *	Do stat
 */
void
fd_stat(struct msg *m, struct file *f)
{
	char buf[MAXSTAT];
	int size, ino;
	char type;
	struct floppy *fl = NULL;


	if (!(f->f_flags & ACC_READ)) {
		msg_err(m->m_sender, EPERM);
		return;
	}
	if (f->f_slot == ROOTDIR) {
		int x;
		
		size = 0;
		ino = 0;
		type = 'd';
		
		/*
		 * Work out the dir size
		 */
		for (x = 0; x < NFD; x++) {
			if (floppies[x].f_state != F_NXIO) {
				int y;
				struct floppy *fl = &floppies[x];
				
				size++;
				for (y = 0; fl->f_posdens[y] != -1; y++) {
					size++;
				}
			}
		}
	} else {
		fl = unit(f->f_unit);
		size = fl->f_parms.f_size;
		ino = MKNODE(f->f_unit, f->f_slot) + UNITSTEP;
		type = 's';
	}
	sprintf(buf,
		"size=%d\ntype=%c\nowner=0\ninode=%d\ndev=%d\n" \
		"fdc=%s\nirq=%d\ndma=%d\nbaseio=0x%0x\nretries=%d\n" \
		"messages=%s\n",
		size, type, ino, fdport_name, fdc_names[fdc_type],
		fd_irq, fd_dma, fd_baseio,
		(f->f_slot == ROOTDIR ? fd_retries : fl->f_retries),
		fdm_opts[(f->f_slot == ROOTDIR
			  ? fd_messages : fl->f_messages)]);
	strcat(buf, perm_print(&fd_prot));
	if (f->f_slot != ROOTDIR) {
		/*
		 * If we're a device we report on media changes and
		 * the size of blocks we'll handle
		 */
		sprintf(&buf[strlen(buf)], "blksize=%d\nmediachg=%d\n",
			(size != FD_PUNDEF
			 ? SECSZ(fl->f_parms.f_secsize) : 512),
			fl->f_mediachg);
	}
	if (f->f_slot == SPECIALNODE) {
		/*
		 * If we're the special node we can deal with drive/media
		 * parameters
		 */
		struct fdparms *fp;
		fp = (fl->f_specialdens == DISK_USERDEF)
		     ? &fl->f_userp : &fl->f_parms;

		sprintf(&buf[strlen(buf)], "findparms=%s\n",
			fl->f_specialdens == DISK_USERDEF ? "userdef" : "auto");
		if (fp->f_size == FD_PUNDEF) {
			sprintf(&buf[strlen(buf)], "parms=undefined\n");
		} else {
			sprintf(&buf[strlen(buf)],
				"parms=%d:%d:%d:%d:%d:0x%02x:0x%02x:" \
				"0x%02x:0x%02x:0x%02x:0x%02x\n",
				fp->f_size, fp->f_tracks, fp->f_heads,
				fp->f_sectors, fp->f_stretch, fp->f_gap,
				fp->f_fmtgap, fp->f_rate, fp->f_spec1,
				fp->f_spec2, fp->f_secsize);
		}
	} 
	m->m_buf = buf;
	m->m_buflen = strlen(buf);
	m->m_nseg = 1;
	m->m_arg = m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}


/*
 * scan_parms()
 *	Scan a user defined media parameter spec
 *
 * Returns zero on fail, non-zero for success
 */
static int
scan_parms(char *val, struct fdparms *fdp)
{
	uint tp[11];
			
	if (sscanf(val, "%d:%d:%d:%d:%d:0x%x:0x%x:0x%x:0x%x:0x%x",
		   &tp[0], &tp[1], &tp[2], &tp[3], &tp[4], &tp[5],
		   &tp[6], &tp[7], &tp[8], &tp[9], &tp[10]) != 11) {
		return(0);
	} else {
		fdp->f_size = tp[0];
		fdp->f_tracks = tp[1];
		fdp->f_heads = tp[2];
		fdp->f_sectors = tp[3];
		fdp->f_stretch = tp[4];
		fdp->f_gap = tp[5];
		fdp->f_fmtgap = tp[6];
		fdp->f_rate = tp[7];
		fdp->f_spec1 = tp[8];
		fdp->f_spec2 = tp[9];
		fdp->f_secsize = tp[10];
		return(11);
	}
}


/*
 * fd_wstat()
 *	Allow writing of supported stat messages
 */
void
fd_wstat(struct msg *m, struct file *f)
{
	char *field, *val;
	struct floppy *fl;
	
	fl = unit(f->f_unit);

	/*
	 * See if common handling code can do it
	 */
	if (do_wstat(m, &fd_prot, f->f_flags, &field, &val) == 0) {
		return;
	}

	/*
	 * Process each of the fields we handle specially here
	 */
	if (!strcmp(field, "mediachg") && (f->f_slot != ROOTDIR)) {
		/*
		 * Number of times we've detected media changes
		 */
		int chg;
		
		if (val) {
			chg = atoi(val);
		} else {
			chg = 0;
		}
		fl->f_mediachg = chg;
	} else if (!strcmp(field, "retries")) {
		/*
		 * Number of times we attempt to retry an operation
		 */
		int retr, x;

		if (val) {
			retr = atoi(val);
		} else {
			retr = FD_MAXERR;
		}
		if (f->f_slot == ROOTDIR) {
			/*
			 * Set the global retry count
			 */
			fd_retries = retr;
			for (x = 0; x < NFD; x++) {
				if (floppies[x].f_state != F_NXIO) {
					floppies[x].f_retries = retr;
				}
			}
		} else {
			/*
			 * Set the specific drive's retry count
			 */
			fl->f_retries = retr;
		}
	} else if (!strcmp(field, "messages")) {
		/*
		 * When will we issue syslog messages?
		 */
		int x, fdm = -1;

		if (val) {
			for (fdm = FDM_ALL; fdm <= FDM_CRITICAL + 1; fdm++) {
				if (fdm > FDM_CRITICAL) {
					/*
					 * Ehh?
					 */
					msg_err(m->m_sender, EINVAL);
					return;
				}
				if (!strcmp(val, fdm_opts[fdm])) {
					break;
				}
			}
		} else {
			fdm = FDM_FAIL;
		}
		if (f->f_slot == ROOTDIR) {
			/*
			 * Set the global messaging level
			 */
			fd_messages = fdm;
			for (x = 0; x < NFD; x++) {
				if (floppies[x].f_state != F_NXIO) {
					floppies[x].f_messages = fdm;
				}
			}
		} else {
			/*
			 * Set the specific drive's messaging level
			 */
			fl->f_messages = fdm;
		}
	} else if (!strcmp(field, "findparms") 
		   && (f->f_slot == SPECIALNODE)) {
		/*
		 * How are we going to determine the special node's diskette
		 * media parameters?
		 */
		if (!strcmp(val, "auto")) {
			/*
			 * Selected autoprobing of the media details
			 */
			fl->f_density = fl->f_specialdens = DISK_AUTOPROBE;
			fl->f_parms.f_size = FD_PUNDEF;
		} else if (!strcmp(val, "userdef")) {
			/*
			 * Selected user defined parameters
			 */
			fl->f_density = fl->f_specialdens = DISK_USERDEF;
			fl->f_parms = fl->f_userp;
		} else {
			/*
			 * We don't understand that option
			 */
			msg_err(m->m_sender, EINVAL);
			return;
		}
	} else if (!strcmp(field, "parms")
		   && (f->f_slot == SPECIALNODE)) {
		/*
		 * What parameters would the user like?
		 */
		if (!strcmp(val, "undefined")) {
			/*
			 * None at all... effectively disable device
			 */
			fl->f_parms.f_size = FD_PUNDEF;
		} else {
			if (!scan_parms(val, &fl->f_userp)) {
				msg_err(m->m_sender, EINVAL);
				return;
			}
			if (fl->f_density == DISK_USERDEF) {
				fl->f_parms = fl->f_userp;
			}
		}
	} else {
		/*
		 * Not a message we understand
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
@Cleanup and switch to syslog
@
text
@d7 1
d13 1
d17 8
d26 4
a29 1
extern struct fdparms fdparms[];
d41 1
d43 1
d48 4
a51 2
	if (f->f_unit == ROOTDIR) {
		size = NFD;
d54 15
a69 2
		struct floppy *fl;

d71 2
a72 2
		size = fdparms[fl->f_density].f_size * SECSZ,
		ino = f->f_unit+1;
d76 8
a83 1
	 "size=%d\ntype=%c\nowner=0\ninode=%d\n", size, type, ino);
d85 33
d125 33
d166 3
d178 133
a310 1
	 * Otherwise forget it
d312 2
a313 1
	msg_err(m->m_sender, EINVAL);
@


1.3
log
@Source reorg
@
text
@d5 2
@


1.2
log
@New UID code
@
text
@a4 1
#include <fd/fd.h>
d8 1
@


1.1
log
@Initial revision
@
text
@d44 1
a44 1
	 "size=%d\ntype=%c\nowner=1/1\ninode=%d\n", size, type, ino);
@
