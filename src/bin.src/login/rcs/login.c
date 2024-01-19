head	1.11;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.7
	V1_1:1.6
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.11
date	94.10.04.19.49.49;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.05.24.17.12.32;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.06.18.41.40;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.02.28.19.15.30;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.25.20.22.08;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.05.00.44.06;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.05.03.21.32.48;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.12.23.30.52;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.24.19.14.43;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.24.00.40.48;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.20.00.24.34;	author vandys;	state Exp;
branches;
next	;


desc
@Login utility
@


1.11
log
@Handle empty strings more gracefully
@
text
@/*
 * login.c
 *	A login program without an attitude
 */
#include <sys/fs.h>
#include <sys/perm.h>
#include <stdio.h>
#include <termios.h>
#include <passwd.h>
#include <fcntl.h>
#include <mnttab.h>
#include <string.h>

#define BANNER "/vsta/etc/banner"	/* Login banner path */
#define SYSMOUNT "/vsta/etc/fstab"	/* Global mounts */
#define MOUNTRC "mount.rc"		/* Per-user mounts */

/*
 * cat()
 *	Cat file if it exists
 */
static void
cat(char *path)
{
	int fd, x;
	char buf[128];

	if ((fd = open(path, O_READ)) < 0) {
		return;
	}
	while ((x = read(fd, buf, sizeof(buf))) > 0) {
		write(1, buf, x);
	}
	close(fd);
}

/*
 * get_str()
 *	Get a string, honor length, always null-terminate
 */
static void
get_str(char *buf, int buflen, int echo)
{
	int nstar[STRLEN];
	int x;
	char c;
	static char stars[] = "****";

	x = 0;
	buflen -= 1;	/* Leave room for terminating '\0' */
	for (;;) {
		/*
		 * Get next char
		 */
		read(0, &c, sizeof(c));
		c &= 0x7f;

		/*
		 * Data--add to buf if room
		 */
		if ((c > ' ') && (c < 0x7f)) {
			if (x >= buflen) {
				continue;
			}
			buf[x] = c;
			if (echo) {
				write(1, &c, sizeof(c));
			} else {
				int y;

				/*
				 * Echo random number of *'s.  This gives
				 * positive feedback without necessarily
				 * telling a peeper how long the password
				 * is.
				 */
				y = (random() & 0x3)+1;
				write(1, stars, y);
				nstar[x] = y;
			}
			x += 1;
			continue;
		}

		/*
		 * End of line
		 */
		if ((c == '\n') || (c == '\r')) {
			write(1, "\r\n", 2);
			buf[x] = '\0';
			return;
		}

		/*
		 * Backspace
		 */
		if (c == '\b') {
			if (x > 0) {
				int y;

				x -= 1;
				y = (echo ? 1 : nstar[x]);
				while (y > 0) {
					write(1, "\b \b", 3);
					y -= 1;
				}
			}
			continue;
		}

		/*
		 * Erase line
		 */
		if ((c == '') || (c == '')) {
			x = 0;
			write(1, "\\\r\n", 3);
			continue;
		}

		/*
		 * Ignore other control chars
		 */
	}
}

/*
 * login()
 *	Log in as given account
 */
static void
login(struct uinfo *u)
{
	int x;
	port_t port;
	struct perm perm;
	char *p, buf[128];
	extern void zero_ids();

	/*
	 * Activate root abilities
	 */
	zero_ids(&perm, 1);
	perm.perm_len = 0;
	if (perm_ctl(1, &perm, (void *)0) < 0) {
		printf("login: can't enable root\n");
		exit(1);
	}

	/*
	 * Set our ID.  We skip slot 1, which is our superuser slot
	 * used to authorize the manipulation of all others.  Finish
	 * by setting 1--after this, we only hold the abilities of
	 * the user logging on.
	 */
	for (x = 0; x < PROCPERMS; ++x) {
		if (x == 1) {
			continue;
		}
		perm_ctl(x, &u->u_perms[x], (void *)0);
	}

	/*
	 * Initialize our environment.  Slot 0, our default ownership,
	 * is now set, so we will own the nodes which appear.
	 */
	setenv_init(u->u_env);

	/*
	 * Give up our powers
	 */
	perm_ctl(1, &u->u_perms[1], (void *)0);

	/*
	 * Re-initialize.  The /env server otherwise still believes
	 * we have vast privileges.
	 */
	setenv_init(u->u_env);

	/*
	 * Mount system default stuff.  Remove our root-capability
	 * entry from the mount table.
	 */
	port = mount_port("/");
	mount_init(SYSMOUNT);
	umount("/", port);

	/*
	 * Put some stuff into our environment.  We place it in the
	 * common part of our environment so all processes under this
	 * login will share it.
	 */
	sprintf(buf, "/%s/USER", u->u_env);
	setenv(buf, u->u_acct);
	sprintf(buf, "/%s/HOME", u->u_env);
	setenv(buf, u->u_home);

	/*
	 * If we can chdir to their home, set up their mount
	 * environment as requested.
	 */
	if (chdir(u->u_home) >= 0) {
		mount_init(MOUNTRC);
	} else {
		printf("Note: can not chdir to home: %s\n", u->u_home);
	}

	/*
	 * Launch their shell with a leading '-' in argv[0] to flag
	 * a login shell.
	 */
	p = strrchr(u->u_shell, '/');
	if (p && (strlen(p)+4 < sizeof(buf))) {
		sprintf(buf, "-%s", p+1);
		p = buf;
	} else {
		p = u->u_shell;
	}
	execl(u->u_shell, p, (char *)0);
	perror(u->u_shell);
	exit(1);
}

/*
 * do_login()
 *	First pass at getting login info
 */
static void
do_login(void)
{
	char acct[STRLEN], passwd[STRLEN], buf[STRLEN];
	char *p;
	struct uinfo uinfo;

	for (;;) {
		printf("login: "); fflush(stdout);
		get_str(acct, sizeof(acct), 1);
		if (acct[0] != '\0') {
			break;
		}
	}
	if (getuinfo_name(acct, &uinfo)) {
		printf("Error; unknown account '%s'\n", acct);
		return;
	}
	if (!strcmp(uinfo.u_passwd, "*")) {
		printf("That account is disabled.\n");
		return;
	}
	if (uinfo.u_passwd[0]) {
		printf("password: "); fflush(stdout);
		get_str(passwd, sizeof(passwd), 0);
		if (strcmp(uinfo.u_passwd, passwd)) {
			printf("Incorrect password.\n");
			return;
		}
	}
	login(&uinfo);
}

/*
 * init_tty()
 *	Set up TTY for raw non-echo input
 */
static void
init_tty(void)
{
	struct termios t;

	tcgetattr(1, &t);
	t.c_lflag &= ~(ICANON|ECHO);
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(1, TCSANOW, &t);
}

main(int argc, char **argv)
{
	extern port_t path_open(char *, int);

	/*
	 * Optionally let them specify device(s)
	 */
	if (argc > 1) {
		char *in, *out;
		port_t p;

		/*
		 * 1st arg is in/out, or in if there's a second
		 */
		in = argv[1];
		if (argc > 2) {
			out = argv[2];
		} else {
			out = in;
		}

		/*
		 * Access named server, then open as input
		 */
		p = path_open(in, ACC_READ);
		if (p < 0) { perror(in); exit(1); }
		close(0);
		__fd_alloc(p);
		
		/*
		 * Open stdout
		 */
		p = path_open(out, ACC_WRITE);
		if (p < 0) { perror(out); exit(1); }
		close(1);
		__fd_alloc(p);

		/*
		 * And stderr--all set to go now
		 */
		close(2);
		dup(1);
	}

	srandom(time((long *)0));
	cat(BANNER);
	init_tty();
	for (;;) {
		do_login();
	}
}
@


1.10
log
@Don't ask for password if it's empty
@
text
@d234 7
a240 2
	printf("login: "); fflush(stdout);
	get_str(acct, sizeof(acct), 1);
@


1.9
log
@Move fs/root to /
@
text
@d244 7
a250 5
	printf("password: "); fflush(stdout);
	get_str(passwd, sizeof(passwd), 0);
	if (strcmp(uinfo.u_passwd, passwd)) {
		printf("Incorrect password.\n");
		return;
@


1.8
log
@Convert to shared port handling code
@
text
@d183 1
a183 1
	port = mount_port("/vsta");
d185 1
a185 1
	umount("/vsta", port);
@


1.7
log
@Allow argument to specify stdin/out/err device
@
text
@d271 2
a277 1
		port_name pn;
d293 1
a293 3
		pn = namer_find(in);
		if (pn < 0) { perror(in); exit(1); }
		p = msg_connect(pn, ACC_READ);
d301 1
a301 3
		pn = namer_find(out);
		if (pn < 0) { perror(out); exit(1); }
		p = msg_connect(pn, ACC_WRITE);
@


1.6
log
@Honor convention of putting leading dash into argv[0]
of login shell.
@
text
@d5 1
d269 1
a269 1
main()
d271 45
@


1.5
log
@Fix lack of star count when echoing
@
text
@d11 1
d135 1
a135 1
	char buf[128];
d207 2
a208 1
	 * Launch their shell
d210 8
a217 1
	execl(u->u_shell, u->u_shell, (char *)0);
@


1.4
log
@Use zero_ids(); it knows about initializing UID field also
@
text
@d100 2
a101 1
				for (y = 0; y < nstar[x]; ++y) {
d103 1
@


1.3
log
@Fix setting of perms.  Fix setting of user-global environment
variables; ".." wouldn't work, due to lexical behavior of
".." in a path.
@
text
@d133 1
d138 1
@


1.2
log
@Lots of fiddling with order of things--need to be careful about
what IDs we have when connecting to a server.  Add two mount
databases--one for global system, second for user.
@
text
@d132 1
d149 1
a149 1
	for (x = 1; x < PROCPERMS; ++x) {
d165 1
a165 1
	perm_ctl(1, &u->u_perms[0], (void *)0);
d186 4
a189 2
	setenv("../USER", u->u_acct);
	setenv("../HOME", u->u_home);
@


1.1
log
@Initial revision
@
text
@d13 2
a14 1
#define MOUNTRC "mount.rc"		/* Per-user desired mounts */
d63 1
a63 1
			buf[x++] = c;
d79 1
d130 2
d134 10
a143 1
	 * Set our ID.  We skip slot 0, which is our superuser slot
d145 1
a145 1
	 * by setting 0--after this, we only hold the abilities of
d149 3
d154 11
a164 1
	perm_ctl(0, &u->u_perms[0], (void *)0);
d167 2
a168 1
	 * Initialize our environment
d173 8
d193 1
a193 1
		init_mount(MOUNTRC);
a230 1
		printf("Got '%s' wanted '%s'\n", passwd, uinfo.u_passwd);
@
