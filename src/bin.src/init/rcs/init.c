head	1.12;
access;
symbols
	V1_3_1:1.12
	V1_3:1.12
	V1_2:1.8
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.12
date	94.04.06.18.41.52;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.03.07.17.50.54;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.02.28.22.07.33;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.02.02.19.42.24;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.28.03.05.53;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.43.08;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.07.09.18.35.46;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.13.17.13.36;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.01.18.49.33;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.24.00.39.24;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.22.23.23.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.22.22.19.34;	author vandys;	state Exp;
branches;
next	;


desc
@Init program
@


1.12
log
@Move fs/root to /
@
text
@/*
 * init.c
 *	Do initial program fire-up
 *
 * This includes mounting default filesystems and then working
 * out way through the init table, running stuff as needed.
 *
 * We go out of our way to read all input in units of lines; our
 * C library "get line" routines understand both VSTa (\n) and
 * DOS (\r\n) end-of-line conventions, and map to just '\n' for us.
 */
#include <sys/types.h>
#include <sys/ports.h>
#include <sys/wait.h>
#include <sys/fs.h>
#include <stdio.h>
#include <std.h>
#include <sys/namer.h>
#include <ctype.h>
#include <syslog.h>

#define INITTAB "/vsta/etc/inittab"	/* Table of stuff to run */

/*
 * State associated with each entry in the inittab
 */
struct inittab {
	uint i_flags;		/* Flags for this entry */
	uint i_when;		/* When to fire it */
	ulong i_pid;		/* PID active for this slot */
	char **i_args;		/* Command+args for this slot */
};

/*
 * Bits for i_flags
 */
#define I_RAN 1		/* Entry has been run */
#define I_RUNNING 2	/* Entry is running now */

/*
 * Values for i_when
 */
#define I_BG 1		/* Run once in background */
#define I_FG 2		/* Run once in foreground */
#define I_AGAIN 3	/* Run in background, restart on exit */

/*
 * Our init tab entries and count
 */
static struct inittab *inittab = 0;
static int ninit = 0;

/*
 * read_inittab()
 *	Parse inittab and store in-core
 */
static void
read_inittab(void)
{
	FILE *fp;
	char *q, *p, buf[80], **argv;
	struct inittab *i = NULL;
	int argc;

	/*
	 * Open inittab.  Bail to standalone shell if missing.
	 */
	if ((fp = fopen(INITTAB, "r")) == NULL) {
		syslog(LOG_ERR, "init: %s: can't open inittab", INITTAB);
		execl("/vsta/bin/testsh", "testsh", (char *)0);
		exit(1);
	}

	/*
	 * Read lines and parse
	 */
	while (fgets(buf, sizeof(buf)-1, fp)) {

		/*
		 * Skip comments
		 */
		if ((buf[0] == '\n') || (buf[0] == '#')) {
			continue;
		}

		/*
		 * Burst line, get another inittab entry
		 */
		buf[strlen(buf)-1] = '\0';
		p = strchr(buf, ':');
		if (p == 0) {
			printf("init: malformed line: %s\n", buf);
			continue;
		}
		*p++ = '\0';
		if (*p == '\0') {
			printf("init: missing command on line: %s\n", buf);
			continue;
		}
		ninit += 1;
		inittab = realloc(inittab, sizeof(struct inittab) * ninit);
		if (inittab == 0) {
			printf("init: out of memory reading inittab\n");
			exit(1);
		}
		i = &inittab[ninit-1];
		bzero(i, sizeof(struct inittab));

		/*
		 * Parse "when"
		 */
		if (!strcmp(buf, "fg")) {
			i->i_when = I_FG;
		} else if (!strcmp(buf, "bg")) {
			i->i_when = I_BG;
		} else if (!strcmp(buf, "again")) {
			i->i_when = I_AGAIN;
		} else {
			printf("init: '%s' uknown, assuming 'bg'\n", buf);
			i->i_when = I_BG;
		}

		/*
		 * Generate args
		 */
		argv = 0;
		argc = 0;
		while (p) {
			/*
			 * Find next word
			 */
			q = strchr(p+1, ' ');
			if (q) {
				*q++ = '\0';
				while (isspace(*q)) {
					++q;
				}
			}
			argc += 1;

			/*
			 * Reallocate argv, allocate a string to hold
			 * the argument.
			 */
			argv = realloc(argv, sizeof(char *) * (argc+1));
			if (argv == 0) {
				printf("init: out of mem for argv\n");
				exit(1);
			}
			argv[argc-1] = strdup(p);
			if (argv[argc-1] == 0) {
				printf("init: out of memory for arg string\n");
				exit(1);
			}

			/*
			 * Advance to next word
			 */
			p = q;
		}

		/*
		 * Last slot is NULL.  We've been making room for it
		 * by always realloc()'ing one too many slots.
		 */
		argv[argc] = NULL;

		/*
		 * Add it to the inittab entry
		 */
		i->i_args = argv;
	}
	fclose(fp);
}

/*
 * launch()
 *	Fire up an entry
 */
static void
launch(struct inittab *i)
{
	long pid;

retry:
	/*
	 * Try and launch off a new process
	 */
	pid = fork();
	if (pid == -1) {
		printf("init: fork failed\n");
		sleep(5);
		goto retry;
	}

	/*
	 * Child--execv() off
	 */
	if (pid == 0) {
		execv(i->i_args[0], i->i_args);
		syslog(LOG_ERR, "init: %s: %s", i->i_args[0], strerror());
		sleep(60);
		_exit(1);
	}

	/*
	 * Parent--mark entry and return
	 */
	i->i_pid = pid;
	i->i_flags = I_RAN|I_RUNNING;
}

/*
 * do_wait()
 *	Wait for a child to die, update our state info
 */
static void
do_wait(void)
{
	struct exitst w;
	int x;

	/*
	 * Wait for child.  If none, pause a sec, then fall out
	 * so our caller can perhaps do something new.
	 */
	if (waits(&w, 1) < 0) {
		sleep(1);
		return;
	}

	/*
	 * Got a dead child.  Riffle through our table and
	 * update whatever entry started him.
	 */
	for (x = 0; x < ninit; ++x) {
		if (inittab[x].i_pid == w.e_pid) {
			inittab[x].i_flags &= ~I_RUNNING;
			break;
		}
	}
}

/*
 * run()
 *	Run an inittab entry
 *
 * This routine handles all the fiddling of inittab entry flags
 * and such.  It also implements the semantics of fg, bg, and again.
 */
static void
run(struct inittab *i)
{
	switch (i->i_when) {
	case I_FG:
	case I_BG:
		/*
		 * One-shot; skip the entry once it's run
		 */
		if (i->i_flags & I_RAN) {
			return;
		}
		launch(i);

		/*
		 * Wait for its death if it's foreground
		 */
		if (i->i_when == I_FG) {
			while (i->i_flags & I_RUNNING) {
				do_wait();
			}
		}
		return;

	case I_AGAIN:
		if (i->i_flags & I_RUNNING) {
			return;
		}
		launch(i);
		return;

	default:
		printf("init: bad i_when\n");
		abort();
	}
}

main()
{
	port_t p;
	port_name pn;

	/*
	 * A moment (2.5 sec) to let servers establish their ports
	 */
	__msleep(2500);

	/*
	 * Connect to console display and keyboard
	 */
	p = path_open("CONS:0", ACC_READ);
	(void)__fd_alloc(p);
	p = path_open("CONS:0", ACC_WRITE);
	(void)__fd_alloc(p);
	(void)__fd_alloc(p);

	/*
	 * Root filesystem
	 */
	for (;;) {
		pn = namer_find("fs/root");
		if (pn < 0) {
			printf("init: can't find root, sleeping\n");
			sleep(5);
		} else {
			break;
		}
	}
	p = msg_connect(pn, ACC_READ);
	if (p < 0) {
		printf("init: can't connect to root\n");
		exit(1);
	}
	mountport("/", p);

	/*
	 * Read in inittab
	 */
	read_inittab();

	/*
	 * Forever run entries from table
	 */
	for (;;) {
		int x;

		for (x = 0; x < ninit; ++x) {
			run(&inittab[x]);
		}
		do_wait();
	}
}
@


1.11
log
@Handle failed exec's on :again: entries more gracefully
@
text
@a22 1
#define FSTAB "/vsta/etc/fstab"		/* Filesystems at boot */
d324 1
a324 1
	mountport("/vsta", p);
@


1.10
log
@Access console using path_open()
@
text
@d20 1
d70 1
a70 1
		perror(INITTAB);
d202 2
a203 1
		perror(i->i_args[0]);
@


1.9
log
@New arg for waits() caught by prototype
@
text
@d300 1
a300 1
	p = msg_connect(PORT_KBD, ACC_READ);
d302 1
a302 1
	p = msg_connect(PORT_CONS, ACC_WRITE);
@


1.8
log
@Add support for "#" comment character
@
text
@d226 1
a226 1
	if (waits(&w) < 0) {
@


1.7
log
@Source reorg
@
text
@d80 7
@


1.6
log
@Boot args work
@
text
@d18 1
a18 1
#include <namer/namer.h>
@


1.5
log
@Set name
@
text
@d280 1
a280 1
main(void)
a283 5

	/*
	 * Fix our name
	 */
	set_cmd("init");
@


1.4
log
@Be a little more forgiving about root FS coming up.
Mostly caused when syscall trace is on in the kernel.
@
text
@d286 5
@


1.3
log
@Big changes, mostly to take out stuff.  Key mistake here was to
establish connections with servers while we're ROOT.  Need to
postpone as much as possible so user can connect to servers
as himself.
@
text
@d302 8
a309 4
	pn = namer_find("fs/root");
	if (pn < 0) {
		printf("init: can't find root\n");
		exit(1);
@


1.2
log
@Fix bunches of stuff, add inittab
@
text
@a169 99
 * mount_fs()
 *	Read initial mount from fstab, put in our mount table
 */
static void
mount_fs(void)
{
	FILE *fp;
	char *r, buf[80], *point, *path;
	port_t p;
	port_name pn;
	int nmount = 0;

	if ((fp = fopen(FSTAB, "r")) == NULL) {
		return;
	}
	while (fgets(buf, sizeof(buf)-1, fp)) {
		/*
		 * Get null-terminated string
		 */
		buf[strlen(buf)-1] = '\0';
		if ((buf[0] == '\0') || (buf[0] == '#')) {
			continue;
		}

		/*
		 * Break into two parts
		 */
		point = strchr(buf, ' ');
		if (point == NULL) {
			continue;
		}
		++point;

		/*
		 * See if we want to walk down into the port
		 * before mounting.
		 */
		path = strchr(buf, ':');
		if (path) {
			*path++ = '\0';
		}

		/*
		 * Look up via namer
		 */
		pn = namer_find(buf);
		if (pn < 0) {
			printf("init: can't find: %s\n", buf);
			continue;
		}
		p = msg_connect(pn, ACC_READ);
		if (p < 0) {
			printf("init: can't connect to: %s\n", buf);
		}

		/*
		 * If there's a path within, walk it now
		 */
		if (path) {
			struct msg m;
			char *q;

			do {
				q = strchr(path, '/');
				if (q) {
					*q++ = '\0';
				}
				m.m_op = FS_OPEN;
				m.m_nseg = 1;
				m.m_buf = path;
				m.m_buflen = strlen(path)+1;
				m.m_arg = ACC_READ;
				m.m_arg1 = 0;
				if (msg_send(p, &m) < 0) {
					printf("Bad path under %s: %s\n",
						buf, path);
					msg_disconnect(p);
					continue;
				}
				path = q;
			} while (path);
		}

		/*
		 * Mount port in its place
		 */
		mountport(point, p);
		if (nmount++ == 0) {
			printf("Mounting:");
		}
		printf(" %s", point); fflush(stdout);
	}
	fclose(fp);
	if (nmount > 0) {
		printf("\n");
	}
}

/*
d286 1
a286 1
	 * A moment (1.5 sec) to let servers establish their ports
a299 16
	 * Mount /namer, /time and /env in their accustomed places
	 */
	p = msg_connect(PORT_NAMER, ACC_READ);
	if (p >= 0) {
		mountport("/namer", p);
	}
	p = msg_connect(PORT_ENV, ACC_READ);
	if (p >= 0) {
		mountport("/env", p);
	}
	p = msg_connect(PORT_TIMER, ACC_READ);
	if (p >= 0) {
		mountport("/time", p);
	}

	/*
a312 10

	/*
	 * Initialize environment
	 */
	setenv_init("");

	/*
	 * Mount others
	 */
	mount_fs();
@


1.1
log
@Initial revision
@
text
@d14 1
d19 1
d25 145
d268 111
d440 6
a445 1
	 * Launch login XXX
d447 8
a454 3
	execl("/vsta/bin/login", "login", (char *)0);
	perror("login");
	exit(1);
@
