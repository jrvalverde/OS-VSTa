head	1.30;
access;
symbols
	V1_3_1:1.27
	V1_3:1.26
	V1_2:1.25
	V1_1:1.25
	V1_0:1.23;
locks; strict;
comment	@ * @;


1.30
date	94.10.01.03.33.47;	author vandys;	state Exp;
branches;
next	1.29;

1.29
date	94.05.30.21.33.14;	author vandys;	state Exp;
branches;
next	1.28;

1.28
date	94.05.24.17.15.42;	author vandys;	state Exp;
branches;
next	1.27;

1.27
date	94.04.26.21.36.17;	author vandys;	state Exp;
branches;
next	1.26;

1.26
date	94.02.28.22.07.11;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	93.10.17.19.26.59;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	93.09.27.23.08.28;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	93.06.30.21.39.53;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	93.04.22.05.10.49;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	93.04.07.21.30.46;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	93.03.26.23.33.41;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	93.03.25.21.30.19;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.03.24.19.14.21;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.03.24.17.43.46;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.03.24.00.38.36;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.03.22.23.22.01;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.03.19.00.57.57;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.03.18.18.21.41;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.03.05.23.20.47;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.03.03.23.19.11;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.02.26.18.47.19;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.02.23.18.24.46;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.02.19.15.37.00;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.02.10.18.11.37;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.02.09.17.12.09;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.02.08.15.11.16;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.05.16.05.02;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.03.20.17.05;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.02.21.18.24;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.21.32;	author vandys;	state Exp;
branches;
next	;


desc
@The test shell
@


1.30
log
@Use function call interfaces to access global C lib data
@
text
@/*
 * main.c
 *	Main routines for test shell
 */
#include <sys/types.h>
#include <sys/fs.h>
#include <mnttab.h>
#include <stdio.h>
#include <ctype.h>
#include <std.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <stat.h>

static void cd(), md(), quit(), ls(), pwd(), do_mount(), cat(), mysleep(),
	sec(), null(), testsh_wstat(), do_fork(), get(), set(),
	do_umount(), rm(), source(), show_mount();
extern void run(), path();
static char *buf;	/* Utility page buffer */

/*
 * For nesting input
 */
#define MAXSTACK (4)
static FILE *instack[MAXSTACK];
static int insp = 0;
static FILE *infile;

/*
 * Table of commands
 */
struct {
	char *c_name;	/* Name of command */
	voidfun c_fn;	/* Function to process the command */
} cmdtab[] = {
	"cat", cat,
	"cd", cd,
	"chdir", cd,
	"env", get,
	"exit", quit,
	"fork", do_fork,
	"fstab", show_mount,
	"get", get,
	"ls", ls,
	"md", md,
	"mkdir", md,
	"mount", do_mount,
	"null", null,
	"path", path,
	"pwd", pwd,
	"quit", quit,
	"rm", rm,
	"run", run,
	"sector", sec,
	"set", set,
	"sleep", mysleep,
	"source", source,
	"umount", do_umount,
	"wstat", testsh_wstat,
	0, 0
};

/*
 * show_mount()
 *	Riffle through mount table, display mounts
 */
static void
show_mount(void)
{
	struct mnttab *m;
	struct mntent *me;
	struct mnttab *__mnttab;
	int __nmnttab;

	__get_mntinfo(&__nmnttab, &__mnttab);
	for (m = __mnttab; m < __mnttab+__nmnttab; ++m) {
		printf("Mounted on %s:\n ", m->m_name);
		for (me = m->m_entries; me; me = me->m_next) {
			printf(" %d", me->m_port);
		}
		printf("\n");
	}
}

/*
 * source()
 *	Push down input a new level
 */
static void
source(char *p)
{
	FILE *fp;

	if (!p || !p[0]) {
		printf("Usage: source <file>\n");
		return;
	}
	if (insp >= MAXSTACK) {
		printf("Too deep\n");
		return;
	}
	fp = fopen(p, "r");
	if (fp == 0) {
		perror(p);
		return;
	}
	instack[insp] = infile;
	insp += 1;
	infile = fp;
}

/*
 * rm()
 *	Remove an entry
 */
static void
rm(char *p)
{
	if (!p || !p[0]) {
		printf("Usage: rm <file>\n");
		return;
	}
	if (unlink(p) < 0) {
		perror(p);
	}
}

/*
 * do_umount()
 *	Unmount all stuff at a point
 */
static void
do_umount(char *p)
{
	int x;

	if (!p || !p[0]) {
		printf("Usage: umount <point>\n");
		return;
	}
	x = umount(p, -1);
	printf("umount returns: %d\n", x);
}

/*
 * get()
 *	Get an environment variable
 */
static void
get(char *p)
{
	char *q;
	extern char *getenv();

	if (!p || !p[0]) {
		printf("Usage: get <var>\n");
		return;
	}
	q = getenv(p);
	if (!q) {
		printf("%s is not set\n", p);
	} else {
		printf("%s=%s\n", p, q);
	}
}

/*
 * set()
 *	Set an environment variable
 */
static void
set(char *p)
{
	char *val;

	if (!p || !p[0]) {
		printf("Usage: set <var> <value>\n");
		return;
	}
	val = strchr(p, ' ');
	if (!val) {
		printf("Missing value\n");
		return;
	}
	*val++ = '\0';
	setenv(p, val);
	printf("%s=%s\n", p, val);
}

/*
 * do_fork()
 *	Fork, and have the child exit immediately
 */
static void
do_fork(void)
{
	int x, y;

	x = fork();
	if (x < 0) {
		perror("fork");
		return;
	}
	if (x == 0) {
		_exit(0);
	}
	y = waits((void *)0);
	printf("Child: %d, return stat %d\n", x, y);
}

/*
 * testsh_wstat()
 *	Write a stat message
 */
static void
testsh_wstat(char *p)
{
	int fd;
	char *q;

	if (!p || !p[0]) {
		printf("Usage: wstat <file> <msg>\n");
		return;
	}
	q = strchr(p, ' ');
	if (!q) {
		printf("Missing message\n");
		return;
	}
	*q++ = '\0';
	fd = open(p, O_RDWR);
	if (fd < 0) {
		perror(p);
		return;
	}
	strcat(q, "\n");
	if (wstat(__fd_port(fd), q) < 0) {
		perror(q);
	}
	close(fd);
}

/*
 * sec()
 *	Dump a sector from a device
 */
static void
sec(char *p)
{
	uint secnum;
	char *secp;
	int fd, x;

	if (!p || !p[0]) {
		printf("Usage is: sector <file> <sector>\n");
		return;
	}

	/*
	 * Parse sector number, default to first sector
	 */
	secp = strchr(p, ' ');
	if (!secp) {
		secnum = 0;
	} else {
		*secp++ = '\0';
		secnum = atoi(secp);
	}

	/*
	 * Open device
	 */
	if ((fd = open(p, 0)) < 0) {
		perror(p);
		return;
	}

	/*
	 * Set file position if needed
	 */
	if (secnum > 0) {
		lseek(fd, 512L * secnum, 0);
	}

	/*
	 * Read a block
	 */
	x = read(fd, buf, 512);
	printf("Read sector %d returns %d\n", secnum, x);
	if (x < 0) {
		perror("read");
	} else {
		extern void dump_s();

		dump_s(buf, (x > 128) ? 128 : x);
	}
	close(fd);
}

/*
 * mysleep()
 *	Pause the requested amount of time
 */
static void
mysleep(char *p)
{
	struct time tm;

	if (!p || !p[0]) {
		printf("Usage: sleep <secs>\n");
		return;
	}
	time_get(&tm);
	printf("Time now: %d sec / %d usec\n", tm.t_sec, tm.t_usec);
	tm.t_sec += atoi(p);
	time_sleep(&tm);
	printf("Back from sleep\n");
}

/*
 * md()
 *	Make a directory
 */
static void
md(char *p)
{
	if (!p || !p[0]) {
		printf("Usage is: mkdir <path>\n");
		return;
	}
	while (isspace(*p)) {
		++p;
	}
	if (mkdir(p) < 0) {
		perror(p);
	}
}

/*
 * null()
 *	File I/O, quiet
 */
static void
null(char *p)
{
	int fd, x;

	if (!p || !p[0]) {
		printf("Usage: null <name>\n");
		return;
	}
	if (!p[0]) {
		printf("Missing filename\n");
		return;
	}
	fd = open(p, O_READ);
	if (fd < 0) {
		perror(p);
		return;
	}
	while ((x = read(fd, buf, NBPG)) > 0) {
		/* write(1, buf, x) */ ;
	}
	close(fd);
}

/*
 * cat()
 *	File I/O
 */
static void
cat(char *p)
{
	char *name;
	int fd, x, output = 0;

	if (!p || !p[0]) {
		printf("Usage: cat [>] <name>\n");
		return;
	}
	if (*p == '>') {
		output = 1;
		++p;
		while (isspace(*p)) {
			++p;
		}
	}
	if (!p[0]) {
		printf("Missing filename\n");
		return;
	}
	name = p;
	fd = open(name, output ? (O_WRITE|O_CREAT) : O_READ, 0);
	if (fd < 0) {
		perror(name);
		return;
	}
	if (output) {
		while ((x = read(0, buf, NBPG)) > 0) {
			if (buf[0] == '\n') {
				break;
			}
			write(fd, buf, x);
		}
		if (x < 0) {
			perror("read");
		}
	} else {
		while ((x = read(fd, buf, NBPG)) > 0) {
			write(1, buf, x);
		}
	}
	close(fd);
}

/*
 * do_mount()
 *	Mount the given port in a slot
 */
static void
do_mount(char *p)
{
	char *path;
	port_t port;
	int x;

	if (!p || !p[0]) {
		printf("Usage: mount <namer-path || port> <mount-point>\n");
	}

	/*
	 * Find the mount point argument
	 */
	path = strchr(p, ' ');
	if (!path) {
		printf("Missing mount point argument\n");
		return;
	}

	/*
	 * Stick a string terminator after our port id
	 */
	*path = '\0';
	path++;

	port = path_open(p, ACC_READ);
	if (port < 0) {
		printf("Can't get connection to server\n");
		return;
	}
	x = mountport(path, port);
	printf("Mount returned %d\n", x);
}

/*
 * quit()
 *	Bye bye
 */
static void
quit(void)
{
	exit(0);
}

/*
 * cd()
 *	Change directory
 */
static void
cd(char *p)
{
	static char *home = 0;
	extern char *getenv();

	if (!p || !p[0]) {
		if (home == 0) {
			p = getenv("HOME");
			if (p == 0) {
				printf("No HOME set\n");
				return;
			}
			home = p;
		} else {
			p = home;
		}
	}
	while (isspace(*p)) {
		++p;
	}
	if (chdir(p) < 0) {
		perror(p);
	} else {
		char buf[128];
		getcwd(buf, sizeof(buf));
		printf("New dir: %s\n", buf);
	}
}

/*
 * pwd()
 *	Print current directory
 */
static void
pwd(void)
{
	char buf[128];

	getcwd(buf, sizeof(buf));
	printf("%s\n", buf);
}

/*
 * ls_l()
 *	Do long listing of an entry
 */
static void
ls_l(char *p)
{
	int fd, first;
	char buf[MAXSTAT];
	struct msg m;
	char *cp;

	fd = open(p, O_READ);
	if (fd < 0) {
		perror(p);
		return;
	}
	m.m_op = FS_STAT|M_READ;
	m.m_buf = buf;
	m.m_arg = m.m_buflen = MAXSTAT;
	m.m_nseg = 1;
	m.m_arg1 = 0;
	if (msg_send(__fd_port(fd), &m) <= 0) {
		printf("%s: stat failed\n", p);
		close(fd);
		return;
	}
	close(fd);
	printf("%s: ", p);
	cp = buf;
	first = 1;
	while (p = strchr(cp, '\n')) {
		*p++ = '\0';
		if (first) {
			first = 0;
		} else {
			printf(", ");
		}
		printf("%s", cp);
		cp = p;
	}
	printf("\n");
}

/*
 * ls()
 *	Print contents of current directory
 */
static void
ls(char *p)
{
	int fd, x, l = 0;
	char buf[256];

	/*
	 * Only -l supported
	 */
	if (p && p[0]) {
		if (strcmp(p, "-l")) {
			printf("Usage: ls [-l]\n");
			return;
		}
		l = 1;
	}

	/*
	 * Open current dir
	 */
	fd = open(".", O_READ);
	if (fd < 0) {
		perror(".");
		return;
	}
	while ((x = read(fd, buf, sizeof(buf)-1)) > 0) {
		char *cp;

		buf[x] = '\0';
		if (!l) {
			printf("%s", buf);
			continue;
		}
		cp = buf;
		while (p = strchr(cp, '\n')) {
			*p++ = '\0';
			ls_l(cp);
			cp = p;
		}
	}
	close(fd);
}

/*
 * do_cmd()
 *	Given command string, look up and invoke handler
 */
static void
do_cmd(str)
	char *str;
{
	int x, len, matches = 0, match;
	char *p;

	p = strchr(str, ' ');
	if (p) {
		len = p-str;
	} else {
		len = strlen(str);
	}

	for (x = 0; cmdtab[x].c_name; ++x) {
		if (!strncmp(cmdtab[x].c_name, str, len)) {
			++matches;
			match = x;
		}
	}
	if (matches == 0) {
		printf("No such command\n");
		return;
	}
	if (matches > 1) {
		printf("Ambiguous\n");
		return;
	}
	(*cmdtab[match].c_fn)(p ? p+1 : p);
}

/*
 * main()
 *	Get started
 */
main(void)
{
#ifdef STAND
	port_t scrn, kbd;

	kbd = path_open("CONS:0", ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = path_open("CONS:0", ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
#endif

	buf = malloc(NBPG);
	if (buf == 0) {
		perror("testsh buffer");
		exit(1);
	}
	if (access("boot.bat", R_OK) >= 0) {
		source("boot.bat");
	}

	infile = stdin;
	for (;;) {
		if (infile == stdin) {
			printf("%% ");
			fflush(stdout);
		}
		if (fgets(buf, NBPG, infile) == 0) {
			if (infile == stdin) {
				printf("<EOF>\n");
				clearerr(infile);
			} else {
				insp -= 1;
				infile = instack[insp];
			}
			continue;
		}
		buf[strlen(buf)-1] = '\0';
		if (buf[0] == '\0') {
			continue;
		}
		do_cmd(buf);
	}
}
@


1.29
log
@Rename function to avoid collision
@
text
@a15 1
extern char *__cwd;	/* Current working dir */
d28 1
a28 1
static FILE *infile = stdin;
d73 2
a74 2
	extern struct mnttab *__mnttab;
	extern int __nmnttab;
d76 1
d494 3
a496 1
		printf("New dir: %s\n", __cwd);
d507 4
a510 1
	printf("%s\n", __cwd);
d581 1
a581 1
	fd = open(__cwd, O_READ);
d583 1
a583 1
		perror(__cwd);
d664 1
@


1.28
log
@Convert to central mount code
@
text
@d14 1
d18 1
a18 1
	sec(), null(), do_wstat(), do_fork(), get(), set(),
d61 1
a61 1
	"wstat", do_wstat,
d213 1
a213 1
 * do_wstat()
d217 1
a217 1
do_wstat(char *p)
@


1.27
log
@Add display of mount table
@
text
@d418 1
a418 1
 *	Mount the given port number in a slot
d423 1
a424 1
	port_name pn;
d428 9
a436 1
		printf("Usage: mount <port> <path>\n");
d439 8
a446 5
	while (isspace(*p)) {
		++p;
	}
	pn = atoi(p);
	port = msg_connect(pn, ACC_READ);
d448 1
a448 1
		perror(p);
d451 2
a452 12
	p = strchr(p, ' ');
	if (!p) {
		printf("Usage: mount <port> <path>\n");
		return;
	}
	while (isspace(*p)) {
		++p;
	}
	x = mountport(p, port);
	if (x < 0) {
		perror(p);
	}
@


1.26
log
@Access console in STAND using new path_open()
@
text
@d7 1
d16 1
a16 1
static void cd(), md(), quit(), ls(), pwd(), mount(), cat(), mysleep(),
d18 1
a18 1
	do_umount(), rm(), source();
d43 1
d48 1
a48 1
	"mount", mount,
d65 21
d417 1
a417 1
 * mount()
d421 1
a421 1
mount(char *p)
@


1.25
log
@Add help instead of bombing
@
text
@a5 3
#ifdef STAND
#include <sys/ports.h>
#endif
d616 1
a616 2
	{
		port_t scrn, kbd;
d618 5
a622 6
		kbd = msg_connect(PORT_KBD, ACC_READ);
		(void)__fd_alloc(kbd);
		scrn = msg_connect(PORT_CONS, ACC_WRITE);
		(void)__fd_alloc(scrn);
		(void)__fd_alloc(scrn);
	}
@


1.24
log
@Fix collision with library-defined sleep()
@
text
@d407 4
@


1.23
log
@Cleanup GCC warnings
@
text
@d18 1
a18 1
static void cd(), md(), quit(), ls(), pwd(), mount(), cat(), sleep(),
d58 1
a58 1
	"sleep", sleep,
d281 1
a281 1
 * sleep()
d285 1
a285 1
sleep(char *p)
@


1.22
log
@Add nesting of I/O and "source" command.  Also implement
a startup/.login type of file.
@
text
@d19 1
a19 1
	sec(), null(), run(), do_wstat(), do_fork(), get(), set(),
@


1.21
log
@Add "rm" command
@
text
@d15 1
d20 1
a20 1
	do_umount(), rm();
d25 8
d59 1
d66 27
d615 2
a616 1
	int scrn, kbd;
d618 6
a623 5
	kbd = msg_connect(PORT_KBD, ACC_READ);
	(void)__fd_alloc(kbd);
	scrn = msg_connect(PORT_CONS, ACC_WRITE);
	(void)__fd_alloc(scrn);
	(void)__fd_alloc(scrn);
d631 3
d636 18
a653 2
		printf("%% "); fflush(stdout);
		gets(buf);
@


1.20
log
@Use perror() when failing a mount
@
text
@d19 1
a19 1
	do_umount();
d45 1
d54 16
@


1.19
log
@GEt rid of STAND default
@
text
@d349 3
a351 1
	int port, x;
d356 2
a357 2
	port = atoi(p);
	port = msg_connect(port, ACC_READ);
d359 1
a359 1
		printf("Bad port.\n");
@


1.18
log
@Make cd without args try to go to home dir
@
text
@a4 1
#define STAND
@


1.17
log
@Move run functions to run.c
@
text
@d392 15
@


1.16
log
@Add interface to fiddle with umount()
@
text
@d21 1
a21 1

d43 1
a167 48
}

/*
 * run()
 *	Fire up an executable
 */
static void
run(char *p)
{
	char *q, **argv;
	int x = 1, bg = 0;
	int pid;

	if (!p || !p[0]) {
		printf("Usage: run <file>\n");
		return;
	}
	if (q = strchr(p, ' ')) {
		*q++ = '\0';
	}
	argv = malloc((x+1) * sizeof(char **));
	argv[0] = p;
	while (q) {
		x += 1;
		argv = realloc(argv, (x+1) * sizeof(char **));
		argv[x-1] = q;
		if (q = strchr(q, ' ')) {
			*q++ = '\0';
		}
	}
	if (!strcmp(argv[x-1], "&")) {
		bg = 1;
		x -= 1;
	}
	argv[x] = 0;
	pid = fork();
	if (pid == 0) {
		x = execv(p, argv);
		perror(p);
		printf("Error code: %d\n", x);
	}
	if (bg) {
		printf("%d &\n", pid);
	} else {
		x = waits((void *)0);
		printf("pid %d leaves status of %d\n", pid, x);
	}
	free(argv);
@


1.15
log
@Add STAND ifdef so testsh will usually inherit stdin/out
@
text
@d19 2
a20 1
	sec(), null(), run(), do_wstat(), do_fork(), get(), set();
d49 1
d53 17
@


1.14
log
@Add environment get/set interface
@
text
@d5 1
d7 1
d9 1
d572 1
d580 1
@


1.13
log
@Add '&' to run in background
@
text
@d16 1
a16 1
	sec(), null(), run(), do_wstat(), do_fork();
d30 1
d33 1
d43 1
d48 45
@


1.12
log
@Add an interface to tickle fork()
@
text
@d107 1
a107 1
	int x = 1;
d127 4
d138 7
a144 2
	x = waits((void *)0);
	printf("pid %d leaves status of %d\n", pid, x);
@


1.11
log
@Convert "run" to fork/exec.  Almost like a REAL shell....
@
text
@d16 1
a16 1
	sec(), null(), run(), do_wstat();
d31 1
d45 21
@


1.10
log
@Add a wstat() test interface (untested as yet), also add
fuller argument vectoring for the "run" command.
@
text
@d86 1
d106 8
a113 3
	x = execv(p, argv);
	perror(p);
	printf("Error code: %d\n", x);
@


1.9
log
@Add "run" command to exec() a new file
@
text
@d16 1
a16 1
	sec(), null(), run();
d41 1
d46 32
d84 2
a85 2
	char *q;
	int x;
d91 1
a91 1
	if ((q = strchr(p, ' '))) {
d94 12
a105 1
	x = execl(p, q, (char *)0);
@


1.8
log
@Add a "silent cat" for just exercising I/O from a device
@
text
@d16 1
a16 1
	sec(), null();
d38 1
d43 22
@


1.7
log
@Move page buffer to common allocation point, add sector dump
with formatted output.
@
text
@d16 1
a16 1
	sec();
d35 1
d140 28
d174 1
a174 1
	char *name, *val;
@


1.6
log
@Do I/O in cat() through page-sized buffer.  This code assumes
a NBPG malloc will be page aligned; it is, with our power-of-two
allocator.  No portable, but this IS just testsh!
@
text
@d15 2
a16 1
static void cd(), md(), quit(), ls(), pwd(), mount(), cat(), sleep();
d18 2
d37 1
d43 57
a146 1
	static char *buf = 0;
a147 7
	if (buf == 0) {
		buf = malloc(NBPG);
		if (buf == 0) {
			perror("cat");
			return;
		}
	}
d176 3
a387 1
	char buf[128];
d395 6
@


1.5
log
@Add a sleep() call
@
text
@d86 1
a86 1
	char buf[128];
d88 7
d117 1
a117 1
		while ((x = read(0, buf, sizeof(buf))) > 0) {
d124 1
a124 1
		while ((x = read(fd, buf, sizeof(buf))) > 0) {
@


1.4
log
@Add ls -l so I can start looking at stat() information
@
text
@d15 1
a15 1
static void cd(), md(), quit(), ls(), pwd(), mount(), cat();
d34 1
d37 20
@


1.3
log
@Replace build hack with a "cat" command
@
text
@d173 44
d221 1
a221 1
ls(void)
d223 1
a223 1
	int fd, x;
d226 14
d241 4
d246 2
d249 10
a258 1
		printf("%s", buf);
@


1.2
log
@Need to add terminating null after read
@
text
@d15 1
a15 1
static void cd(), md(), quit(), ls(), pwd(), mount(), build();
d24 1
a26 1
	"build", build,
d57 2
a58 2
 * build()
 *	Create a namer node
d61 1
a61 1
build(char *p)
d64 2
a65 1
	int fd, x;
d68 1
a68 1
		printf("Usage: build <name> <val>\n");
d71 9
a79 4
	name = p;
	p = strchr(p, ' ');
	if (!p) {
		printf("Missing value\n");
d82 2
a83 6
	*p++ = '\0';
	val = p;
	while (isspace(*val)) {
		++val;
	}
	fd = open(name, O_WRITE|O_CREAT, 0);
d88 12
a99 2
	x = write(fd, val, strlen(val));
	printf("Write of '%s' gives %d\n", val, x);
@


1.1
log
@Initial revision
@
text
@d172 1
@
