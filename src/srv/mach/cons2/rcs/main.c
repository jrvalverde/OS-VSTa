head	1.15;
access;
symbols
	V1_3_1:1.13
	V1_3:1.13
	V1_2:1.6
	V1_1:1.5
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.15
date	94.06.21.20.57.06;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.05.30.21.27.52;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.04.07.00.48.59;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.03.07.17.51.33;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.03.04.17.01.44;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.02.28.22.02.31;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.02.28.19.17.14;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.02.28.04.59.54;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.02.28.04.52.56;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.01.15.02.12.13;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.45.50;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.07.09.18.33.48;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.13.17.11.56;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.43.12;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.42.52;	author vandys;	state Exp;
branches;
next	;


desc
@Main loop
@


1.15
log
@Convert to openlog()
@
text
@/*
 * main.c
 *	Main message handling
 *
 * This handler takes both console (output) and keyboard (input).  It
 * also does the magic necessary to multiplex onto the multiple virtual
 * consoles.
 */
#include <sys/perm.h>
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/ports.h>
#include <sys/syscall.h>
#include <sys/namer.h>
#include <hash.h>
#include "cons.h"
#include <stdio.h>
#include <std.h>
#include <syslog.h>

extern int valid_fname(void *, uint);

static struct hash
	*filehash;	/* Map session->context structure */

port_t consport;	/* Port we receive contacts through */
uint curscreen = 0,	/* Current screen # receiving data */
	hwscreen = 0;	/* Screen # showing on HW */

/*
 * Protection for console; starts out with access for all.  sys can
 * change the protection label.
 */
struct prot cons_prot = {
	1,
	ACC_READ|ACC_WRITE,
	{1},
	{ACC_CHMOD}
};

/*
 * Per-virtual screen state
 */
struct screen screens[NVTY];

/*
 * new_client()
 *	Create new per-connect structure
 */
static void
new_client(struct msg *m)
{
	struct file *f;
	struct perm *perms;
	int uperms, nperms, desired;

	/*
	 * See if they're OK to access
	 */
	perms = (struct perm *)m->m_buf;
	nperms = (m->m_buflen)/sizeof(struct perm);
	uperms = perm_calc(perms, nperms, &cons_prot);
	desired = m->m_arg & (ACC_WRITE|ACC_READ|ACC_CHMOD);
	if ((uperms & desired) != desired) {
		msg_err(m->m_sender, EPERM);
		return;
	}

	/*
	 * Get data structure
	 */
	if ((f = malloc(sizeof(struct file))) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Fill in fields.
	 */
	bzero(f, sizeof(struct file));
	f->f_flags = uperms;
	f->f_screen = ROOTDIR;

	/*
	 * Hash under the sender's handle
	 */
        if (hash_insert(filehash, m->m_sender, f)) {
		free(f);
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Return acceptance
	 */
	msg_accept(m->m_sender);
}

/*
 * dup_client()
 *	Duplicate current file access onto new session
 */
static void
dup_client(struct msg *m, struct file *fold)
{
	struct file *f;

	/*
	 * Get data structure
	 */
	if ((f = malloc(sizeof(struct file))) == 0) {
		msg_err(m->m_sender, strerror());
		return;
	}

	/*
	 * Fill in fields.  Simply duplicate old file.
	 */
	*f = *fold;

	/*
	 * Hash under the sender's handle
	 */
        if (hash_insert(filehash, m->m_arg, f)) {
		free(f);
		msg_err(m->m_sender, ENOMEM);
		return;
	}

	/*
	 * Return acceptance
	 */
	m->m_arg = m->m_arg1 = m->m_buflen = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * dead_client()
 *	Someone has gone away.  Free their info.
 */
static void
dead_client(struct msg *m, struct file *f)
{
	(void)hash_delete(filehash, m->m_sender);
	free(f);
}

/*
 * check_gen()
 *	See if the console is still accessible to them
 */
static int
check_gen(struct msg *m, struct file *f)
{
	if (f->f_gen != screens[f->f_screen].s_gen) {
		msg_err(m->m_sender, EIO);
		return(1);
	}
	return(0);
}

/*
 * do_open()
 *	Open from root to particular device
 */
static void
do_open(struct msg *m, struct file *f)
{
	uint x;
	struct screen *s;

	/*
	 * Must be in root
	 */
	if (f->f_screen != ROOTDIR) {
		msg_err(m->m_sender, EINVAL);
		return;
	}

	/*
	 * Get screen number
	 */
	x = ((char *)m->m_buf)[0] - '0';
	if (x >= NVTY) {
		msg_err(m->m_sender, ESRCH);
		return;
	}

	/*
	 * Allocate screen memory if it isn't there already.  This should
	 * be its first use, so init its queue structure as well.
	 */
	s = &screens[x];
	if (s->s_img == 0) {
		s->s_img = malloc(SCREENMEM);
		if (s->s_img == 0) {
			msg_err(m->m_sender, strerror());
			return;
		}
		ll_init(&s->s_readers);

		/*
		 * Record it as off-screen initially, and start it
		 * as blank.
		 */
		s->s_curimg = s->s_img;
		clear_screen(s->s_img);
	}

	/*
	 * Switch to this screen, and tell them they've succeeded.  Note
	 * that the screen is *not* the HW one until they switch to it.
	 */
	f->f_screen = x;
	m->m_arg = m->m_arg1 = m->m_nseg = 0;
	msg_reply(m->m_sender, m);
}

/*
 * switch_screen()
 *	Input has arrived for a different screen; switch
 *
 * This routine does not flip the visual screens; it causes the
 * pointers in our emulator to point at different buffers.
 *
 * If the new screen is on the hardware, we don't point at its RAM
 * image, we ask for the hardware instead.
 */
static void
switch_screen(uint new)
{
	struct screen *s;

	/*
	 * Save the old screen position.  The data itself isn't saved,
	 * but the display position may have moved and we need to update
	 * our copy.
	 */
	save_screen_pos(&screens[curscreen]);

	/*
	 * Set to the new one, and point the display engine at its
	 * memory.
	 */
	s = &screens[curscreen = new];
	set_screen(s->s_curimg, s->s_pos);
}

/*
 * select_screen()
 *	Switch hardware display to new screen #
 */
void
select_screen(uint new)
{
	struct screen *sold = &screens[hwscreen],
		*snew = &screens[new];

	/*
	 * Don't switch to a screen which has never been opened.  Don't
	 * bother switching to the current (it's a no-op anyway).  Bounce
	 * attempts to switch to an unconfigured screen.
	 */
	if ((new >= NVTY) || (snew->s_img == 0) || (hwscreen == new)) {
		return;
	}

	/*
	 * Save cursor position for curscreen
	 */
	save_screen_pos(&screens[curscreen]);

	/*
	 * Dump HW image into curscreen, and set curscreen to start
	 * doing its I/O to the RAM copy.
	 */
	save_screen(sold);
	sold->s_curimg = sold->s_img;

	/*
	 * Switch to new guy, and tell him to start displaying to HW
	 */
	curscreen = hwscreen = new;
	load_screen(snew);
}

/*
 * do_readdir()
 *	Read from pseudo-dir ROOTDIR
 */
static void
do_readdir(struct msg *m, struct file *f)
{
	static char *mydir;
	static int len;

	/*
	 * Create our "directory" just once, and hold it for further use.
	 * This assumes we never go to three digit VTY numbers.
	 */
	if (mydir == 0) {
		uint x;

		/*
		 * Get space for NVTY sequences of "%2d\n", plus '\0'
		 */
		mydir = malloc(3 * NVTY + 1);
		if (mydir == 0) {
			msg_err(m->m_sender, strerror());
			return;
		}

		/*
		 * Write them on the buffer
		 */
		mydir[0] = '\0';
		for (x = 0; x < NVTY; ++x) {
			sprintf(mydir + strlen(mydir), "%d\n", x);
		}
		len = strlen(mydir);
	}

	if (f->f_pos >= len) {
		/*
		 * If they have it all, return 0 count
		 */
		m->m_arg = m->m_nseg = 0;
	} else {
		/*
		 * Otherwise give them whatever part of "mydir"
		 * they need now.
		 */
		m->m_nseg = 1;
		m->m_buf = mydir + f->f_pos;
		m->m_buflen = len - f->f_pos;
		if (m->m_buflen > m->m_arg) {
			m->m_buflen = m->m_arg;
		}
		m->m_arg = m->m_buflen;
		f->f_pos += m->m_arg;
	}

	/*
	 * Send it back
	 */
	m->m_arg1 = 0;
	msg_reply(m->m_sender, m);
}

/*
 * screen_main()
 *	Endless loop to receive and serve requests
 */
static void
screen_main()
{
	struct msg msg;
	char *buf2 = 0;
	int x;
	struct file *f;

loop:
	/*
	 * Receive a message, log an error and then keep going
	 */
	x = msg_receive(consport, &msg);
	if (x < 0) {
		syslog(LOG_ERR, "msg_receive");
		goto loop;
	}

	/*
	 * If we've received more than a single buffer of data, pull it in
	 * to a dynamic buffer.
	 */
	if (msg.m_nseg > 1) {
		buf2 = malloc(x);
		if (buf2 == 0) {
			msg_err(msg.m_sender, E2BIG);
			goto loop;
		}
		(void)seg_copyin(msg.m_seg, msg.m_nseg, buf2, x);
		msg.m_buf = buf2;
		msg.m_buflen = x;
		msg.m_nseg = 1;
	}

	/*
	 * Categorize by basic message operation
	 */
	f = hash_lookup(filehash, msg.m_sender);
	switch (msg.m_op) {
	case M_CONNECT:		/* New client */
		new_client(&msg);
		break;
	case M_DISCONNECT:	/* Client done */
		dead_client(&msg, f);
		break;
	case M_DUP:		/* File handle dup during exec() */
		dup_client(&msg, f);
		break;
	case M_ABORT:		/* Aborted operation */
		/*
		 * If there's a pending read, abort it.  Writes are
		 * synchronous.
		 */
		if (f->f_readcnt) {
			abort_read(f);
			f->f_readcnt = 0;
		}
		msg_reply(msg.m_sender, &msg);
		break;

	case FS_READ:		/* Read "directory" or keyboard */
		if (f->f_screen == ROOTDIR) {
			do_readdir(&msg, f);
		} else {
			if (check_gen(&msg, f)) {
				break;
			}
			kbd_read(&msg, f);
		}
		break;

	case FS_WRITE:		/* Write file */
		/*
		 * If this is I/O to a different display than was
		 * last rendered via write_string(), tell it to switch
		 * over.
		 */
		if (curscreen != f->f_screen) {
			switch_screen(f->f_screen);
		}

		/*
		 * Check access generation
		 */
		if (check_gen(&msg, f)) {
			break;
		}

		/*
		 * Write data
		 */
		if (msg.m_buflen > 0) {
			/*
			 * Scribble the bytes onto the display, be it
			 * the HW or our RAM image.
			 */
			write_string(msg.m_buf, msg.m_buflen);

			/*
			 * If this screen is the one on the hardware,
			 * update the HW cursor.
			 */
			if (curscreen == hwscreen) {
				cursor();
			}
		}
		msg.m_arg = x;
		msg.m_buflen = msg.m_arg1 = msg.m_nseg = 0;
		msg_reply(msg.m_sender, &msg);
		break;

	case FS_STAT:		/* Stat of file */
		if (check_gen(&msg, f)) {
			break;
		}
		cons_stat(&msg, f);
		break;
	case FS_WSTAT:		/* Write selected stat fields */
		if (f->f_screen == ROOTDIR) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		if (check_gen(&msg, f)) {
			break;
		}
		cons_wstat(&msg, f);
		break;
	case FS_OPEN:		/* Open particular screen device */
		if (!valid_fname(msg.m_buf, msg.m_buflen)) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}
		do_open(&msg, f);
		break;

	case M_ISR:		/* Interrupt */
		kbd_isr(&msg);
		break;

	default:		/* Unknown */
		msg_err(msg.m_sender, EINVAL);
		break;
	}

	/*
	 * Free dynamic storage if in use
	 */
	if (buf2) {
		free(buf2);
		buf2 = 0;
	}
	goto loop;
}

#ifdef DEBUG
/*
 * do_dbg_enter()
 *	Drop into kernel debugger
 *
 * Save/restore screen so on-screen kernel debugger won't mess up
 * our screen.
 */
void
do_dbg_enter(void)
{
	extern void dbg_enter(void);

	save_screen_pos(&screens[curscreen]);
	save_screen(&screens[hwscreen]);
	dbg_enter();
	load_screen(&screens[hwscreen]);
}
#endif

/*
 * main()
 *	Startup of the screen server
 */
int
main(int argc, char **argv)
{
	int vid_type = VID_CGA;
	int i;

	/*
	 * Initialize syslog
	 */
	openlog("cons", LOG_PID, LOG_DAEMON);

	/*
	 * First let's parse any command line options
	 */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-color")) {
			vid_type = VID_CGA;
		}
		else if (!strcmp(argv[i], "-colour")) {
			vid_type = VID_CGA;
		}
		else if (!strcmp(argv[i], "-mono")) {
			vid_type = VID_MGA;
		}
	}

	/*
	 * Allocate handle->file hash table.  16 is just a guess
	 * as to what we'll have to handle.
	 */
        filehash = hash_alloc(16);
	if (filehash == 0) {
		syslog(LOG_ERR, "file hash not allocated");
		exit(1);
        }

	/*
	 * Turn on our I/O access
	 */
	if (enable_io(CONS_LOW, CONS_HIGH) < 0) {
		syslog(LOG_ERR, "can't do display I/O operations");
		exit(1);
	}
	if (enable_io(KEYBD_LOW, KEYBD_HIGH) < 0) {
		syslog(LOG_ERR, "can't do keyboard I/O operations");
		exit(1);
	}

	/*
	 * Get a port for the console
	 */
	consport = msg_port(PORT_CONS, 0);
	(void)namer_register("tty/cons", PORT_CONS);

	/*
	 * Tell system about our I/O vector
	 */
	if (enable_isr(consport, KEYBD_IRQ)) {
		syslog(LOG_ERR, "can't get keyboard IRQ %d", KEYBD_IRQ);
		exit(1);
	}

	/*
	 * Let screen mapping get initialized
	 */
	init_screen(vid_type);

	/*
	 * Allocate memory for screen 0, the current screen.  Mark
	 * him as currently using the hardware.
	 */
	screens[0].s_img = malloc(SCREENMEM);
	if (screens[0].s_img == 0) {
		syslog(LOG_ERR, "can't allocated screen #0 image");
		exit(1);
	}
	screens[0].s_curimg = hw_screen;
	ll_init(&screens[0].s_readers);

	/*
	 * Start serving requests for the filesystem
	 */
	screen_main();
	return(0);
}
@


1.14
log
@Syslog support
@
text
@d27 1
a27 3
char cons_sysmsg[] = "cons (CONS):";
uint
	curscreen = 0,	/* Current screen # receiving data */
d368 1
a368 1
		syslog(LOG_ERR, "%s msg_receive", cons_sysmsg);
d539 5
d564 1
a564 1
		syslog(LOG_ERR, "%s file hash not allocated", cons_sysmsg);
d572 1
a572 2
		syslog(LOG_ERR, "%s can't do display I/O operations",
			cons_sysmsg);
d576 1
a576 2
		syslog(LOG_ERR, "%s can't do keyboard I/O operations",
			cons_sysmsg);
d590 1
a590 2
		syslog(LOG_ERR, "%s can't get keyboard IRQ %d",
			cons_sysmsg, KEYBD_IRQ);
d605 1
a605 2
		syslog(LOG_ERR, "%s can't allocated screen #0 image",
			cons_sysmsg);
@


1.13
log
@Add support for -mono and -color
@
text
@d27 1
d370 1
a370 1
		syslog(LOG_ERR, "cons: msg_receive");
d561 1
a561 1
		syslog(LOG_ERR, "cons: file hash");
d569 2
a570 1
		syslog(LOG_ERR, "cons: can't do I/O operations");
d574 2
a575 1
		syslog(LOG_ERR, "cons/kbd: can't do I/O operations");
d589 2
a590 1
		syslog(LOG_ERR, "cons: Keyboard IRQ");
d605 2
a606 1
		syslog(LOG_ERR, "cons: Screen #0 image");
@


1.12
log
@Fix screen save when jumping into kernel debugger
@
text
@d534 1
a534 1
main()
d536 18
d593 1
a593 1
	init_screen();
@


1.11
log
@Get multiple active I/O screens working
@
text
@d522 2
a523 1
	save_screen(&screens[curscreen]);
d525 1
a525 1
	load_screen(&screens[curscreen]);
@


1.10
log
@Convert to syslog()
@
text
@d257 1
a257 1
	struct screen *sold = &screens[curscreen],
d265 1
a265 1
	if ((new >= NVTY) || (snew->s_img == 0) || (new == curscreen)) {
d270 5
a285 1
	snew->s_curimg = hw_screen;
a426 11

		/*
		 * We redirect writes to ROOTDIR into screen #0.  This
		 * is just for compatibility with logging messages during
		 * bootup.
		 */
		if (f->f_screen == ROOTDIR) {
			msg_err(msg.m_sender, EINVAL);
			break;
		}

@


1.9
log
@Get working, add screen cleanup for kernel debugger hook
@
text
@d19 1
d356 1
a356 1
	int x, tmpscreen;
d365 1
a365 1
		perror("cons: msg_receive");
d430 2
a431 3
			tmpscreen = 0;
		} else {
			tmpscreen = f->f_screen;
d439 2
a440 2
		if (curscreen != tmpscreen) {
			switch_screen(tmpscreen);
d548 1
a548 1
		perror("file hash");
d556 1
a556 1
		perror("CONS: can't do I/O operations");
d560 1
a560 1
		perror("KBD: can't do I/O operations");
d574 1
a574 1
		perror("Keyboard IRQ");
d589 1
a589 1
		perror("Screen #0 image");
@


1.8
log
@Add better checking; convert to per-screen access generation counter
@
text
@d44 1
a44 1
static struct screen screens[NVTY];
d80 1
a80 1
	f->f_gen = 0;
a82 1
	f->f_pos = 0;
d207 1
a207 1
		bzero(s->s_img, SCREENMEM);
d261 2
a262 1
	 * bother switching to the current (it's a no-op anyway).
d264 1
a264 1
	if ((snew->s_img == 0) || (new == curscreen)) {
d355 1
a355 1
	int x;
d400 2
a401 2
		 * We're synchronous, so presumably the operation
		 * is all done and this abort is old news.
d403 4
d424 3
a426 1
		 * Can't write dir
d429 3
a431 2
			msg_err(msg.m_sender, EINVAL);
			break;
d439 2
a440 2
		if (curscreen != f->f_screen) {
			switch_screen(f->f_screen);
d496 5
d515 19
@


1.7
log
@Modify main handling to do multiple virtual consoles; this code
started with plain old cons-type driver code, and has had the kbd
code added in as well.
@
text
@d26 1
a26 1
uint accgen = 0,	/* Generation counter for access */
d80 1
a80 1
	f->f_gen = accgen;
d156 1
a156 1
	if (f->f_gen != accgen) {
a406 3
		if (check_gen(&msg, f)) {
			break;
		}
d410 3
d418 6
a423 1
		if (check_gen(&msg, f)) {
d437 7
d473 4
@


1.6
log
@Cleanup
@
text
@d4 4
d13 2
d18 1
d20 1
a20 2
extern void write_string(), *malloc(), cons_stat(), cons_wstat();
extern char *strerror();
d22 2
a23 1
struct hash *filehash;	/* Map session->context structure */
d26 3
a28 1
uint accgen = 0;	/* Generation counter for access */
d42 5
d82 2
d153 1
a153 1
static
d164 183
a353 1
	seg_t resid;
d405 12
d421 13
d435 4
d440 8
d449 1
a450 1
		msg.m_arg = x;
d453 1
d466 7
d492 1
a494 2
	extern void init_screen();

d509 5
a513 1
		fprintf(stderr, "CONS: can't do I/O operations\n");
d521 9
d537 12
d552 1
@


1.5
log
@Source reorg
@
text
@d166 1
a166 1
		perror("bfs: msg_receive");
@


1.4
log
@Boot args work
@
text
@d9 2
a10 2
#include <lib/hash.h>
#include <cons/cons.h>
@


1.3
log
@Set name
@
text
@d249 1
a249 1
main(int argc, char **argv)
a251 7

	/*
	 * Set name if boot module
	 */
	if (argc == 0) {
		set_cmd("cons");
	}
@


1.2
log
@New arg for msg_port
@
text
@d249 1
a249 1
main()
d252 7
@


1.1
log
@Initial revision
@
text
@d274 1
a274 1
	consport = msg_port(PORT_CONS);
@
