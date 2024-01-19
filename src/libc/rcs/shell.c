head	1.5;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.3
	V1_1:1.3
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.5
date	94.10.28.04.44.54;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	94.10.13.14.17.59;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.12.20.55.31;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.09.17.12.02;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.08.23.03.20;	author vandys;	state Exp;
branches;
next	;


desc
@system() interface; wrapper for fork()/execv()
@


1.5
log
@Fiddle const declarations, very intuitive
@
text
@/*
 * shell.c
 *	Shell interface stuff
 */
#include <sys/wait.h>
#include <std.h>

/*
 * system()
 *	Launch a shell, return status
 */
int
system(const char *cmd)
{
	char *argv[4];
	long pid;
	struct exitst w;

	argv[0] = "sh";
	argv[1] = "-c";
	argv[2] = (char *)cmd;
	argv[3] = 0;
	pid = fork();
	if (pid < 0) {
		return(-1);
	}
	if (pid == 0) {
		execv("/vsta/bin/sh", argv);
		_exit(-1);
	}
	waits(&w, 1);
	return(w.e_code);
}

/*
 * execvp()
 *	Interface to execv() with path honored
 */
execvp(const char *prog, char * const *argv)
{
	char *pathbuf, *path, *p, *buf;
	int len;

	/*
	 * Absolute doesn't use path
	 */
	if ((prog[0] == '/') || !strncmp(prog, "./", 2) ||
			!strncmp(prog, "../", 3)) {
		return(execv(prog, argv));
	}

	/*
	 * Try to find path from environment.  Just use program
	 * name as-is if we don't have a path.
	 */
	pathbuf = path = getenv("PATH");
	if (path == 0) {
		return(execv(prog, argv));
	}

	/*
	 * Try each element
	 */
	len = strlen(prog);
	do {
		/*
		 * Find next path element
		 */
		p = strchr(path, ';');
		if (p == 0) {
			p = strchr(path, ':');
		}
		if (p) {
			*p++ = '\0';
		}

		/*
		 * Get temp buffer for full path
		 */
		buf = malloc(len+strlen(path)+1);
		if (buf == 0) {
			return(-1);
		}
		sprintf(buf, "%s/%s", path, prog);
		(void)execv(buf, argv);

		/*
		 * Didn't fly; free buffer and iterate
		 */
		free(buf);
		path = p;
	} while (path);

	/*
	 * All failed.  Free malloc()'ed buffer of PATH, and return
	 * failure.
	 */
	free(pathbuf);
	return(-1);
}

/*
 * execlp()
 *	execl(), with path
 */
execlp(const char *path, const char *arg0, ...)
{
	return(execvp(path, (char **)&arg0));
}
@


1.4
log
@Add some const declarations
@
text
@d15 1
a15 1
	const char *argv[4];
d21 1
a21 1
	argv[2] = cmd;
d39 1
a39 1
execvp(const char *prog, const char **argv)
d108 1
a108 1
	return(execvp(path, &arg0));
@


1.3
log
@Add path-oriented functions
@
text
@d12 2
a13 1
system(char *cmd)
d15 1
a15 1
	char *argv[4];
d39 1
a39 1
execvp(char *prog, char **argv)
d106 1
a106 1
execlp(char *path, char *arg0, ...)
@


1.2
log
@Add new arg to waits()
@
text
@d6 1
d27 1
a27 1
		execv("/bin/sh", argv);
d32 76
@


1.1
log
@Initial revision
@
text
@d29 1
a29 1
	waits(&w);
@
