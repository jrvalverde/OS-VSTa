head	1.9;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.5
	V1_1:1.5
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.9
date	94.10.06.01.56.15;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.06.21.20.57.06;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.11.00.35.47;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.04.02.02.21;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.45.20;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.10.08.02.26.38;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.02.20.16.45;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.16.14.11.03;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.12.19.56.48;	author vandys;	state Exp;
branches;
next	;


desc
@High-level I/O setup--partitions, offsets, and I/O queues.
@


1.9
log
@Add support for multiple controllers and such
@
text
@/*
 * rw.c
 *	Reads and writes to the hard disk
 */
#include <stdio.h>
#include <sys/fs.h>
#include <llist.h>
#include <sys/assert.h>
#include <stdlib.h>
#include <syslog.h>
#include "wd.h"

/*
 * Busy/waiter flags
 */
int busy;			/* Busy, and unit # */
int busy_unit;
struct llist waiters;		/* Who's waiting */
char *secbuf = NULL;

static int update_partition_list(int, int);

/*
 * queue_io()
 *	Record parameters of I/O, queue to unit
 */
static int
queue_io(struct msg *m, struct file *f)
{
	uint unit, cnt;
	ulong part_off;

	unit = NODE_UNIT(f->f_node);
	ASSERT_DEBUG(unit < NWD, "queue_io: bad unit");

	/*
	 * Get a block offset based on partition
	 */
	switch (dpart_get_offset(disks[unit].d_parts, NODE_SLOT(f->f_node),
				 f->f_pos / SECSZ, &part_off, &cnt)) {
	case 0:			/* Everything's OK */
		break;
	case 1:			/* At end of partition (EOF) */
		m->m_nseg = m->m_arg = m->m_arg1 = 0;
		msg_reply(m->m_sender, m);
		return(1);
	case 2:			/* Illegal offset */
		msg_err(m->m_sender, EINVAL);
		return(1);
	}

	/*
	 * If they didn't provide a buffer, generate one for
	 * ourselves.
	 */
	if (m->m_nseg == 0) {
		f->f_buf = malloc(m->m_arg);
		if (f->f_buf == 0) {
			msg_err(m->m_sender, ENOMEM);
			return(1);
		}
		f->f_local = 1;
	} else {
		f->f_buf = m->m_buf;
		f->f_local = 0;
	}
	f->f_count = m->m_arg / SECSZ;
	if (f->f_count > cnt) {
		f->f_count = cnt;
	}
	f->f_unit = unit;
	f->f_blkno = part_off;
	f->f_op = m->m_op;
	if ((f->f_list = ll_insert(&waiters, f)) == 0) {
		if (f->f_local) {
			free(f->f_buf);
		}
		msg_err(m->m_sender, ENOMEM);
		return(1);
	}
	return(0);
}

/*
 * run_queue()
 *	If there's stuff in queue, launch next
 *
 * If there's nothing, clears "busy"
 */
static void
run_queue(void)
{
	struct file *f;

	/*
	 * Nobody waiting--disk falls idle
	 */
	if (waiters.l_forw == &waiters) {
		busy = 0;
		return;
	}

	/*
	 * Remove next from list
	 */
	f = waiters.l_forw->l_data;
	ll_delete(waiters.l_forw);

	/*
	 * Launch I/O
	 */
	busy = 1;
	busy_unit = f->f_unit;
	wd_io(f->f_op, f, f->f_unit, f->f_blkno, f->f_buf, f->f_count);
}

/*
 * wd_rw()
 *	Do I/O to the disk
 *
 * m_arg specifies how much they want.  It must be in increments
 * of sector sizes, or we EINVAL'em out of here.
 */
void
wd_rw(struct msg *m, struct file *f)
{
	/*
	 * Sanity check operations on directories
	 */
	if (m->m_op == FS_READ) {
		if (f->f_node == ROOTDIR) {
			wd_readdir(m, f);
			return;
		}
	} else {
		/* FS_WRITE */
		if ((f->f_node == ROOTDIR) || (m->m_nseg != 1)) {
			msg_err(m->m_sender, EINVAL);
			return;
		}
	}

	/*
	 * Check size of I/O request
	 */
	if ((m->m_arg > MAXIO) || (m->m_arg <= 0)) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Check alignment of request (block alignment)
	 */
	if ((m->m_arg & (SECSZ - 1)) || (f->f_pos & (SECSZ-1))) {
		msg_err(m->m_sender, EBALIGN);
		return;
	}

	/*
	 * Check permission
	 */
	if (((m->m_op == FS_READ) && !(f->f_flags & ACC_READ)) ||
			((m->m_op == FS_WRITE) && !(f->f_flags & ACC_WRITE))) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Queue I/O to unit
	 */
	if (!queue_io(m, f) && !busy) {
		run_queue();
	}
}

/*
 * iodone()
 *	Called from disk level when an I/O is completed
 *
 * This routine has two modes; before partundef is 0, we are just iteratively
 * reading first sectors of configured disks, and firing up our partition
 * interpreter to get partitioning information.  Once we've done this for all
 * disks, this routine becomes your standard "finish one, start another"
 * handler.
 */
void
iodone(void *tran, int result)
{
	uint unit;
	struct file *f;

	ASSERT_DEBUG(tran != 0, "iodone: null tran");

	/*
	 * Special case when the partition entries are undefined
	 */
	if (partundef) {
		/*
		 * "tran" is just our unit #, casted
		 */
		unit = ((uint)tran) - 1;
		ASSERT(unit < NWD, "iodone: partundef bad unit");
		if (update_partition_list(unit, 0) == 0) {
			return;
		}

		/*
		 * OK, have we now read all of the partition data?
		 */
		if (partundef) {
			int i;

			for (i = 0; i < NWD; i++) {
				if (partundef & (1 << i)) {
					if (disks[i].d_configed) {
						update_partition_list(i, 1);
						return;
					} else {
						partundef &= (0xffffffff
							      ^ (1 << i));
					}
				}
			}		
		}
		return;
	}

	/*
	 * I/O completion
	 */
	f = tran;
	if (result == -1) {
		/*
		 * I/O error; complete this operation
		 */
		msg_err(f->f_sender, EIO);
	} else {
		struct msg m;

		/*
		 * Success.  Return count of bytes processed and
		 * buffer if local.  Update the f_pos pointer to show where
		 * we've reached so far
		 */
		m.m_arg = f->f_count * SECSZ;
		f->f_pos += m.m_arg;

		if (f->f_local) {
			m.m_nseg = 1;
			m.m_buf = f->f_buf;
			m.m_buflen = m.m_arg;
		} else {
			m.m_nseg = 0;
		}
		m.m_arg1 = 0;
		msg_reply(f->f_sender, &m);
	}

	/*
	 * Free local buffer, clear busy field
	 */
	if (f->f_local) {
		free(f->f_buf);
	}
	f->f_list = 0;

	/*
	 * Run next request, if any
	 */
	run_queue();
}

/*
 * rw_init()
 *	Initialize our queue data structure
 */
void
rw_init(void)
{
	ll_init(&waiters);
}


/*
 * rw_readpartitions()
 *	Read the drive partition tables for the specified disk
 *
 * We start with the assumption that any parameters we already have must be
 * invalidated, so we perform the invalidation first.  Note that if other
 * partition lists have been invalidated elsewhere that these will be
 * refetched when we fetch these
 */
void
rw_readpartitions(int unit)
{
	/*
	 * First, bounce any requests to non-configured drives
	 */
	if (!disks[unit].d_configed) {
		return;
	}

	/*
	 * Ensure that we have a suitable sector buffer with which to work
	 */
	if (secbuf == NULL) {
		secbuf = malloc(SECSZ * 2);
		if (secbuf == NULL) {
			syslog(LOG_ERR, "sector buffer");
			return;
		}
		secbuf = (char *)roundup((long)secbuf, SECSZ);
	}

	/*
	 * Reset the partition valid flag for this unit
	 */
	partundef |= (1 << unit);

	/*
	 * We now issue successive calls to read any unknown partition
	 * information
	 */
	update_partition_list(unit, 1);
}


/*
 * update_partition_list()
 *	Update the partition data for the specified disk unit
 *
 * This routine is called to carry out the management of disk partition
 * data.  It is responsible for issuing read requests and tracking which
 * device is being talked to
 *
 * Returns 1 when the update is complete, 0 if it's only partway done.
 */
static int
update_partition_list(int unit, int initiating)
{
	static uint sector_num;
	static int next_part;

	/*
	 * Are we initiating a partition read?
	 */
	if (initiating) {
		/*
		 * OK, zero down the reference markers, establish the
		 * "whole disk" parameters and do the 1st read
		 */
		dpart_init_whole("wd", unit, disks[unit].d_parm.w_size,
				 &wd_prot, disks[unit].d_parts);
		sector_num = 0;
		next_part = FIRST_PART;
		wd_io(FS_READ, (void *)(unit + 1), unit, 0L, secbuf, 1);
		return 0;
	} else {
		/*
		 * We must be in the middle of an update so sort out the
		 * table manipulations and start any further reads
		 */
		if (dpart_init("wd", (uint)unit, secbuf,
			       &sector_num, &wd_prot,
			       disks[unit].d_parts, &next_part) == 0) {
			sector_num = 0;
		}
		if (sector_num != 0) {
			wd_io(FS_READ, (void *)(unit + 1), unit,
			      sector_num, secbuf, 1);
			return 0;
		} else {
			partundef &= (0xffffffff ^ (1 << unit));
			return 1;
	        }
	}
}
@


1.8
log
@Convert to openlog()
@
text
@a12 2
extern void wd_readdir();

a20 6
extern uint partundef;		/* All partitioning read yet? */
extern char configed[];
extern struct disk disks[];
extern struct wdparms parm[];


a22 1

d144 1
a144 1
	 * Check size and alignment
d146 1
a146 4
	if ((m->m_arg & (SECSZ - 1)) ||
			(m->m_arg > MAXIO) ||
			(m->m_arg == 0) ||
			(f->f_pos & (SECSZ-1))) {
d152 8
d215 1
a215 1
					if (configed[i]) {
d242 2
a243 1
		 * buffer if local.
d246 2
d299 1
a299 1
	if (!configed[unit]) {
d352 2
a353 2
		dpart_init_whole("wd", unit, parm[unit].w_size,
				 disks[unit].d_parts);
d360 1
a360 1
		 * We must bein the middle of an update so sort out the
d363 2
a364 1
		if (dpart_init("wd", (uint)unit, secbuf, &sector_num,
@


1.7
log
@Fix warnings
@
text
@d310 1
a310 1
			syslog(LOG_ERR, "wd: sector buffer");
@


1.6
log
@Convert to -ldpart
@
text
@d342 2
a343 1
	static int sector_num, next_part;
d364 1
a364 1
		if (dpart_init("wd", unit, secbuf, &sector_num,
@


1.5
log
@Source reorg
@
text
@d5 1
d9 2
a10 1
#include <std.h>
d18 1
a18 1
int busy;		/* Busy, and unit # */
d20 2
a21 1
struct llist waiters;	/* Who's waiting */
d23 4
a26 1
extern int upyet;	/* All partitioning read yet? */
d28 4
d36 1
a36 1
static
d48 2
a49 1
	switch (get_offset(f->f_node, f->f_pos / SECSZ, &part_off, &cnt)) {
d155 1
a155 1
	if ((m->m_arg & (SECSZ-1)) ||
d184 5
a188 5
 * This routine has two modes; before upyet, we are just iteratively
 * reading first sectors of configured disks, and firing up our
 * partition interpreter to get partitioning information.  Once
 * we've done this for all disks, this routine becomes your standard
 * "finish one, start another" handler.
a192 1
	int x;
d199 1
a199 1
	 * Special case before we're "upyet"
d201 1
a201 3
	if (!upyet) {
		extern char *secbuf;

d206 4
a209 2
		ASSERT(unit < NWD, "iodone: !upyet bad unit");
		init_part(unit, secbuf);
d212 1
a212 1
		 * Find next unit to process
d214 14
a227 6
		for (x = unit+1; x < NWD; ++x) {
			if (configed[x]) {
				wd_io(FS_READ, (void *)(x+1),
					x, 0L, secbuf, 1);
				return;
			}
a228 5

		/*
		 * We've covered all--ready for clients!
		 */
		upyet = 1;
d282 94
@


1.4
log
@Protect against zero-length reads
@
text
@a4 1
#include <wd/wd.h>
d6 1
a6 1
#include <lib/llist.h>
d9 1
@


1.3
log
@configed[] defined in .h now
@
text
@d146 1
@


1.2
log
@Add code for I/O completion and partitioning
@
text
@a191 1
		extern int configed[];
@


1.1
log
@Initial revision
@
text
@d27 1
a27 1
queue_io(uint unit, struct msg *m, struct file *f)
d29 2
a30 1
	ASSERT_DEBUG(unix < NWD, "queue_io: bad unit");
d32 18
d58 1
a58 1
			return;
d65 4
a68 1
	f->f_count = m->m_arg/SECSZ;
d70 1
a70 1
	f->f_blkno = f->f_pos/SECSZ;
d89 1
a89 1
run_queue()
d142 1
a142 1
	 * Check size
d163 1
a163 1
	if (!queue_io(NODE_UNIT(f->f_node), m, f) && !busy) {
d183 1
d185 5
d218 41
@
