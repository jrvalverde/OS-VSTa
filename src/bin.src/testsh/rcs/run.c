head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.5
date	94.02.02.19.42.24;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.06.30.21.39.53;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.22.00.20.48;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.30.01.12.24;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.24.17.43.16;	author vandys;	state Exp;
branches;
next	;


desc
@Routines for searching and running executables
@


1.5
log
@New arg for waits() caught by prototype
@
text
@/*
 * run.c
 *	Routines for running an external program
 */
#include <std.h>
#include <stdio.h>
#include <sys/wait.h>

static char *curpath = NULL;

/*
 * execvp2()
 *	Exec using search path
 *
 * No return on success; returns -1 on failure to run.
 */
static
execvp2(char *prog, char **argv)
{
	char *p, *q;
	char path[128];

	/*
	 * If has leading '/' or './' or '../' then use path as-is
	 */
	if ((prog[0] == '/') || !strncmp(prog, "./", 2) ||
			!strncmp(prog, "../", 3)) {
		return(execv(path, argv));
	}

	/*
	 * Otherwise try each prefix in current path and try to
	 * find an executable.
	 */
	p = curpath;
	while (p) {
		/*
		 * Find next path element seperator, copy into place
		 */
		q = strchr(p, ':');
		if (q) {
			bcopy(p, path, q-p);
			path[q-p] = '\0';
			++q;
		} else {
			strcpy(path, p);
		}
		sprintf(path+strlen(path), "/%s", prog);

		/*
		 * Try to run
		 */
		execv(path, argv);

		/*
		 * Advance to next element or end
		 */
		p = q;
	}
	return(-1);
}

/*
 * run()
 *	Fire up an executable
 */
void
run(char *p)
{
	char *q, **argv;
	int x = 1, bg = 0;
	int pid;
	char buf[128];
	struct exitst e;

	if (!p || !p[0]) {
		printf("Usage: run <file>\n");
		return;
	}

	/*
	 * Chop up into arguments
	 */
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

	/*
	 * Trailing &--run in background
	 */
	if (!strcmp(argv[x-1], "&")) {
		bg = 1;
		x -= 1;
	}

	/*
	 * Null pointer terminates
	 */
	argv[x] = 0;

	/*
	 * Launch child
	 */
	pid = fork();
	if (pid == 0) {
		x = execvp2(p, argv);
		perror(p);
		printf("Error code: %d\n", x);
	}
	if (bg) {
		printf("%d &\n", pid);
	} else {
		for (;;) {
			e.e_pid = 0;
			x = waits(&e, 1);
			if (e.e_pid == 0) {
				perror("waits");
				break;
			}
			printf("pid %d status %d user %d system %d\n",
				pid, e.e_code, e.e_usr, e.e_sys);
			if (e.e_pid == pid) {
				break;
			}
		}
	}
	free(argv);
}

/*
 * path()
 *	Set search path for executable
 */
void
path(char *p)
{
	if (!p || !p[0]) {
		printf("Path: %s\n", curpath ? curpath : "not set");
		return;
	}
	if (curpath) {
		free(curpath);
	}
	curpath = strdup(p);
#ifndef STAND
	/*
	 * For child processes who use the regular execvp()
	 */
	setenv("PATH", curpath);
#endif
}
@


1.4
log
@Cleanup GCC warnings
@
text
@d125 1
a125 1
			x = waits(&e);
@


1.3
log
@Set PATH environment variable if not standalone
@
text
@d12 1
a12 1
 * execvp()
d18 1
a18 1
execvp(char *prog, char **argv)
d116 1
a116 1
		x = execvp(p, argv);
@


1.2
log
@Reap return results until we get our own PID.
@
text
@d155 6
@


1.1
log
@Initial revision
@
text
@d123 13
a135 3
		x = waits(&e);
		printf("pid %d status %d user %d system %d\n",
			pid, e.e_code, e.e_usr, e.e_sys);
@
