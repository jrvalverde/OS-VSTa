head	1.7;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.7
date	94.10.12.04.00.29;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.08.25.00.57.41;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.05.24.17.07.05;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.03.00.00.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.19.00.56.20;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.18.15.46;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.02.27;	author vandys;	state Exp;
branches;
next	;


desc
@POSIX termios emulation in user mode
@


1.7
log
@Add getline() for canonical input processing
@
text
@/*
 * tty.c
 *	Routines for doing I/O to TTY-like devices
 *
 * In VSTa, the TTY driver knows nothing about line editing and
 * so forth--the user code handles it instead.  These are the default
 * routines, which do most of what you'd expect.  They use a POSIX
 * TTY interface and do editing, modes, and so forth right here.
 *
 * This module assumes there's just a single TTY.  It gets harder
 * to associate state back to various file descriptors.  We could
 * stat the device and hash on device/inode, but that would have to
 * be done per fillbuf, which seems wasteful.  This should suffice
 * for most applications; I'll think of something for the real
 * solution.
 */
#include <sys/fs.h>
#include <termios.h>
#include <fdl.h>
#include <std.h>
#include <stdio.h>
#include "getline.h"

#define PROMPT_SIZE (80)	/* Max bytes expected in prompt */

/*
 * Current tty state, initialized for line-by-line
 */
static struct termios tty_state = {
	ISTRIP|ICRNL,		/* c_iflag */
	OPOST|ONLCR,		/* c_oflag */
	CS8|CREAD|CLOCAL,	/* c_cflag */
	ECHOE|ECHOK|ECHO|
		ISIG|ICANON,	/* c_lflag */
				/* c_cc[] */
{'\4', '\n', '\10', '\27', '\30', '\3', '\37', '\21', '\23', '\1', 0},
	B9600,			/* c_ispeed */
	B9600			/* c_ospeed */
};

/*
 * Where we stash unconsumed TTY data
 */
typedef struct {
	uint f_cnt;
	char *f_pos;
	uint f_bufsz;
	char f_buf[BUFSIZ];
} TTYBUF;

/*
 * This is global instead of within the TTYBUF, as it needs to
 * be coordinated between stdin and stdout.
 */
static char f_prompt[PROMPT_SIZE];

/*
 * init_port()
 *	Create TTYBUF for per-TTY state
 */
static
init_port(struct port *port)
{
	TTYBUF *t;

	t = port->p_data = malloc(sizeof(TTYBUF));
	if (!t) {
		return(1);
	}
	t->f_cnt = 0;
	t->f_bufsz = BUFSIZ;
	t->f_pos = t->f_buf;
	return(0);
}

/*
 * canon()
 *	Do input processing when canonical input is set
 */
static int
canon(struct termios *t, TTYBUF *fp, struct port *port)
{
	char *prompt;

	/*
	 * Calculate the prompt by digging through the last
	 * write() to see what would be on the current output
	 * line.
	 */
	prompt = strrchr(f_prompt, '\n');
	if (prompt) {
		prompt += 1;
	} else {
		prompt = f_prompt;
	}

	/*
	 * Use getline() to fill the buffer.  Add to history.
	 * Note that we keep our own f_buf, but don't use it
	 * since getline() gave us his.
	 */
	fp->f_pos = getline(prompt);
	fp->f_cnt = strlen(fp->f_pos);
	if (fp->f_cnt > 0) {
		gl_histadd(fp->f_pos);
	}

	return(0);
}

/*
 * non_canon()
 *	Do input when ICANON turned off
 *
 * This mode is complex and wonderful.  The code here does its best
 * for the common cases (especially VMIN==1, VTIME==0!) but does not
 * pretend to handle all those timing-sensitive modes.
 */
static int
non_canon(struct termios *t, TTYBUF *fp, struct port *port)
{
	int x;
	struct msg m;

	/*
	 * Set up
	 */
	fp->f_cnt = 0;
	fp->f_pos = fp->f_buf;

	/*
	 * Unlimited time, potentially limited data
	 */
	if (t->c_cc[VTIME] == 0) {
		/*
		 * Build request.  Read as much as we can get.
		 */
		m.m_op = FS_READ|M_READ;
		m.m_buf = fp->f_buf;
		if (t->c_cc[VMIN] == 0) {
			m.m_arg = 0;
		} else {
			m.m_arg = BUFSIZ;
		}
		m.m_buflen = BUFSIZ;
		m.m_nseg = 1;
		m.m_arg1 = 0;
		x = msg_send(port->p_port, &m);
		if (x <= 0) {
			return(-1);
		}

		/*
		 * Flag data buffered.  If ECHO, echo it back now.
		 */
		fp->f_cnt += x;
		if (t->c_lflag & ECHO) {
			(void)write(1, fp->f_buf, x);
		}
		return(0);
	}

	/*
	 * Others not supported (yet)
	 */
	return(-1);
}

/*
 * Macro to do one-time setup of a file descriptor for a TTY
 */
#define SETUP(port) \
	if (port->p_data == 0) { \
		if (init_port(port)) { \
			return(-1); \
		} \
	}

/*
 * __tty_read()
 *	Fill buffer from TTY-type device
 */
__tty_read(struct port *port, void *buf, uint nbyte)
{
	struct termios *t = &tty_state;
	TTYBUF *fp;
	int error, cnt;

	/*
	 * Do one-time setup if needed, get pointer to state info
	 */
	SETUP(port);
	fp = port->p_data;

	/*
	 * Load next buffer if needed
	 */
	if (fp->f_cnt == 0) {
		/*
		 * Non-canonical processing get its own routine
		 */
		if ((t->c_lflag & ICANON) == 0) {
			error = non_canon(t, fp, port);
		} else {
			error = canon(t, fp, port);
		}
	}

	/*
	 * I/O errors get caught here
	 */
	if (error && (fp->f_cnt == 0)) {
		return(-1);
	}

	/*
	 * Now that we have a buffer with the needed bytes, copy
	 * out as many as will fit and are available.
	 */
	cnt = MIN(fp->f_cnt, nbyte);
	bcopy(fp->f_pos, buf, cnt);
	fp->f_pos += cnt;
	fp->f_cnt -= cnt;

	return(cnt);
}

/*
 * __tty_write()
 *	Flush buffers to a TTY-type device
 *
 * XXX add the needed CRNL conversion and such.
 */
__tty_write(struct port *port, void *buf, uint nbyte)
{
	struct msg m;
	TTYBUF *fp;
	int ret;

	/*
	 * Display string.  Shortcut out if not doing
	 * canonical input processing
	 */
	m.m_op = FS_WRITE;
	m.m_buf = buf;
	m.m_arg = m.m_buflen = nbyte;
	m.m_nseg = 1;
	m.m_arg1 = 0;
	ret = msg_send(port->p_port, &m);
	if ((ret < 0) || ((tty_state.c_lflag & ICANON) == 0)) {
		return(ret);
	}

	/*
	 * Get buffering for prompt, get pointer to port state
	 */
	SETUP(port);
	fp = port->p_data;

	/*
	 * Keep track of byte displayed to cursor on this line.
	 * Needed to provide prompt string for input editing. Bleh.
	 * Note we assume that the last write() will be (at least) the
	 * entire prompt.  We leave digging around for the \n until we
	 * really need to prompt, favoring speeds of write()'s to TTYs
	 * over speed to set up a getline().
	 *
	 * f_prompt is actually PROMPT_SIZE+1 bytes, so there IS
	 * room for the \0 termination.
	 */
	if (nbyte > PROMPT_SIZE) {
		buf = (char *)buf + (nbyte - PROMPT_SIZE);
		nbyte = PROMPT_SIZE;
	}
	bcopy(buf, f_prompt, nbyte);
	f_prompt[nbyte] = '\0';

	return(ret);
}

/*
 * __tty_close()
 *	Free our typing buffer
 */
__tty_close(struct port *port)
{
	free(port->p_data);
	return(0);
}

/*
 * tcsetattr()
 *	Set TTY attributes
 *
 * This needs rethinking when I put a TTY monitor on top.  We still
 * fiddle the state in the FDL, but we also have to pass some stuff
 * up to our monitor.  For instance, the monitor needs to know what
 * the interrupt characters are so he can spot them and send a signal
 * to the appropriate process group.  wstat() will be the right way,
 * but have to think what format the wstat() message would use, so leave
 * until I'm ready to work on the monitor code.
 */
tcsetattr(int fd, int flag, struct termios *t)
{
	struct port *port;

	port = __port(fd);
	if (!port || (port->p_read != __tty_read)) {
		return(-1);
	}
	tty_state = *t;
	return(0);
}

/*
 * tcgetattr()
 *	Get current TTY attributes
 */
tcgetattr(int fd, struct termios *t)
{
	struct port *port;

	port = __port(fd);
	if (!port || (port->p_read != __tty_read)) {
		return(-1);
	}
	*t = tty_state;
	return(0);
}

/*
 * cfgetispeed()
 *	Return TTY baud rate
 */
ulong
cfgetispeed(struct termios *t)
{
	return(t->c_ispeed);
}

/*
 * cfgetospeed()
 *	Return TTY baud rate
 */
ulong
cfgetospeed(struct termios *t)
{
	return(t->c_ospeed);
}

/*
 * cfsetispeed()
 *	Set TTY input baud rate
 */
int
cfsetispeed(struct termios *t, ulong speed)
{
	t->c_ispeed = speed;
	return(0);
}

/*
 * cfsetospeed()
 *	Set TTY output baud rate
 */
cfsetospeed(struct termios *t, ulong speed)
{
	t->c_ospeed = speed;
	return(0);
}
@


1.6
log
@Add cfset[io]speed
@
text
@d22 1
d24 2
d52 2
a53 25
 * addc()
 *	Stuff another character into the buffer
 */
static void
addc(TTYBUF *fp, char c)
{
	if (fp->f_cnt < fp->f_bufsz) {
		fp->f_buf[fp->f_cnt] = c;
		fp->f_cnt += 1;
	}
}

/*
 * delc()
 *	Remove last character from buffer
 */
static void
delc(TTYBUF *fp)
{
	fp->f_cnt -= 1;
}

/*
 * echo()
 *	Dump bytes directly to TTY
d55 1
a55 5
static void
echo(char *p)
{
	write(1, p, strlen(p));
}
a76 10
 * Macro to do one-time setup of a file descriptor for a TTY
 */
#define SETUP(port) \
	if (port->p_data == 0) { \
		if (init_port(port)) { \
			return(-1); \
		} \
	}

/*
d80 1
a80 1
static
d83 1
a83 3
	char c, c2;
	char echobuf[2];
	struct msg m;
d86 3
a88 1
	 * Get ready to read from scratch
d90 6
a95 3
	echobuf[1] = 0;
	fp->f_cnt = 0;
	fp->f_pos = fp->f_buf;
d98 3
a100 1
	 * Loop getting characters
d102 5
a106 63
	for (;;) {
		/*
		 * Build request
		 */
		m.m_op = FS_READ|M_READ;
		m.m_buf = &c2;
		m.m_arg = m.m_buflen = sizeof(c2);
		m.m_nseg = 1;
		m.m_arg1 = 0;

		/*
		 * Errors are handled below.  Otherwise we
		 * move to a local variable so the optimizer
		 * can leave it in a register if it likes.
		 */
		if (msg_send(port->p_port, &m) != sizeof(c2)) {
			return(-1);
		}
		c = c2;

		/*
		 * Null char--always store.  Null in c_cc[] means
		 * an * inactive entry, so null characters would
		 * cause some confusion.
		 */
		if (!c) {
			addc(fp, c);
			continue;
		}

		/*
		 * ICRNL--map CR to NL
		 */
		if ((c == '\r') && (t->c_iflag & ICRNL)) {
			c = '\n';
		}

		/*
		 * Erase?
		 */
		if (c == t->c_cc[VERASE]) {
			if (fp->f_cnt < 1) {
				continue;
			}
			delc(fp);
			echo("\b \b");	/* Not right for tab */
			continue;
		}

		/*
		 * Kill?
		 */
		if (c == t->c_cc[VKILL]) {
			echo("\\\r\n");	/* Should be optional */
			fp->f_cnt = 0;
			fp->f_pos = fp->f_buf;
			continue;
		}

		/*
		 * Add the character
		 */
		addc(fp, c);
a107 15
		/*
		 * Echo?
		 */
		if (t->c_lflag & ECHO) {
			echobuf[0] = c;
			echo(echobuf);
		}

		/*
		 * End of line?
		 */
		if (c == t->c_cc[VEOL]) {
			break;
		}
	}
d119 1
a119 1
static
a122 1
	char echobuf[2];
a127 1
	echobuf[1] = 0;
d170 10
d237 2
d240 4
d249 30
a278 1
	return(msg_send(port->p_port, &m));
@


1.5
log
@Support VMIN == 0 for non-blocking I/O
@
text
@d409 21
@


1.4
log
@Add cfget[oi]speed
@
text
@d244 6
a249 1
		m.m_arg = m.m_buflen = BUFSIZ;
@


1.3
log
@First pass at !ICANON handling; restructuring to accomodate
cooked and raw; add tcget/setattr.
@
text
@d384 20
@


1.2
log
@Fix backspacing--f_pos should just stay pointing at
start of buffer.
@
text
@d111 2
a112 2
 * __tty_read()
 *	Fill buffer from TTY-type device
d114 2
a115 1
__tty_read(struct port *port, void *buf, uint nbyte)
a117 1
	struct termios *t = &tty_state;
a118 2
	TTYBUF *fp;
	uint cnt = 0;
a119 1
	int error = 0;
d122 1
a122 1
	 * Do one-time setup if needed, get pointer to state info
d124 3
a126 2
	SETUP(port);
	fp = port->p_data;
d129 1
a129 1
	 * Load next buffer if needed
d131 30
a160 1
	if (fp->f_cnt == 0) {
d162 1
a162 1
		 * Get ready to read from scratch
d164 3
a166 27
		echobuf[1] = 0;
		fp->f_cnt = 0;
		fp->f_pos = fp->f_buf;

		/*
		 * Loop getting characters
		 */
		for (;;) {
			/*
			 * Build request
			 */
			m.m_op = FS_READ|M_READ;
			m.m_buf = &c2;
			m.m_arg = m.m_buflen = sizeof(c2);
			m.m_nseg = 1;
			m.m_arg1 = 0;

			/*
			 * Errors are handled below.  Otherwise we
			 * move to a local variable so the optimizer
			 * can leave it in a register if it likes.
			 */
			if (msg_send(port->p_port, &m) != sizeof(c2)) {
				error = -1;
				break;
			}
			c = c2;
d168 5
a172 7
			/*
			 * Null char--always store.  Null in c_cc[] means
			 * an * inactive entry, so null characters would
			 * cause some confusion.
			 */
			if (!c) {
				addc(fp, c);
d175 59
d235 16
a250 6
			/*
			 * ICRNL--map CR to NL
			 */
			if ((c == '\r') && (t->c_iflag & ICRNL)) {
				c = '\n';
			}
d252 9
a260 11
			/*
			 * Erase?
			 */
			if (c == t->c_cc[VERASE]) {
				if (fp->f_cnt < 1) {
					continue;
				}
				delc(fp);
				echo("\b \b");	/* Not right for tab */
				continue;
			}
d262 5
a266 9
			/*
			 * Kill?
			 */
			if (c == t->c_cc[VKILL]) {
				echo("\\\r\n");	/* Should be optional */
				fp->f_cnt = 0;
				fp->f_pos = fp->f_buf;
				continue;
			}
d268 9
a276 4
			/*
			 * Add the character
			 */
			addc(fp, c);
d278 5
a282 7
			/*
			 * Echo?
			 */
			if (t->c_lflag & ECHO) {
				echobuf[0] = c;
				echo(echobuf);
			}
d284 11
a294 6
			/*
			 * End of line?
			 */
			if (c == t->c_cc[VEOL]) {
				break;
			}
d342 40
@


1.1
log
@Initial revision
@
text
@a67 1
	fp->f_pos -= 1;
@
