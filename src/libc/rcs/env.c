head	1.5;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.5
date	94.10.13.14.17.59;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.24.00.37.16;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.20.00.22.12;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.19.00.56.50;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.13.01.30.45;	author vandys;	state Exp;
branches;
next	;


desc
@Mapping of getenv/setenv into /env server
@


1.5
log
@Add some const declarations
@
text
@/*
 * env.c
 *	Give a getenv/setenv environment from the /env server
 *
 * These routines assume the /env server is indeed mounted
 * at /env in the user's mount table.
 */
#include <std.h>
#include <fcntl.h>
#include <sys/fs.h>
#include <sys/ports.h>

extern char *rstat();

/*
 * getenv()
 *	Ask /env server for value
 */
char *
getenv(const char *name)
{
	char *p, buf[256];
	int fd, len;

	/*
	 * Slashes in path mean absolute path to environment
	 * variable.
	 */
	if (strchr(name, '/')) {
		while (name[0] == '/') {
			++name;
		}
		sprintf(buf, "/env/%s", name);
	} else {
		/*
		 * Otherwise use our "home" node
		 */
		sprintf(buf, "/env/#/%s", name);
	}

	/*
	 * Access name, return 0 if not found
	 */
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		return(0);
	}

	/*
	 * Get # byte needed to represent, allocate buffer, and
	 * read into the buffer.
	 */
	p = rstat(__fd_port(fd), "size");
	if (p == 0) {
		close(fd);
		return(0);
	}
	len = atoi(p);
	p = malloc(len+1);
	p[len] = '\0';
	(void)read(fd, p, len);
	close(fd);
	return(p);
}

/*
 * setenv()
 *	Set variable in environment to value
 */
setenv(const char *name, const char *val)
{
	char buf[256];
	int fd, len;

	/*
	 * Slashes in path mean absolute path to environment
	 * variable.
	 */
	if (strchr(name, '/')) {
		while (name[0] == '/') {
			++name;
		}
		sprintf(buf, "/env/%s", name);
	} else {
		/*
		 * Otherwise use our "home" node
		 */
		sprintf(buf, "/env/#/%s", name);
	}

	/*
	 * Write name, create as needed
	 */
	fd = open(buf, O_WRITE|O_CREAT, 0600);
	if (fd < 0) {
		return(-1);
	}

	/*
	 * Write value
	 */
	len = strlen(val);
	if (write(fd, val, len) != len) {
		return(-1);
	}
	close(fd);

	return(0);
}

/*
 * setenv_init()
 *	Initialize environment
 *
 * Usually called after a login to establish a new base and
 * copy-on-write node.  Returns 0 on success, -1 on failure.
 */
setenv_init(char *base)
{
	char buf[80];
	int fd;
	port_t port;
	char *p, *q;

	/*
	 * Connect afresh to the server
	 */
	port = msg_connect(PORT_ENV, ACC_READ);
	if (port < 0) {
		return(-1);
	}

	/*
	 * Throw away the current /env mount
	 */
	umount("/env", -1);

	/*
	 * Mount our new /env point
	 */
	if (mountport("/env", port) < 0) {
		return(-1);
	}

	/*
	 * Put home in root or where specified
	 */
	if (base) {
		while (base[0] == '/') {
			++base;
		}
		sprintf(buf, "/env/%s/#", base);
	} else {
		sprintf(buf, "/env/#");
	}

	/*
	 * Skip first two slashes (from above; they should
	 * definitely exist)
	 */
	p = strchr(buf, '/');
	p = strchr(p, '/');
	++p;

	/*
	 * mkdir successive elements
	 */
	do {
		p = strchr(p, '/');
		if (p) {
			*p = '\0';
		}
		if ((fd = open(buf, O_READ)) < 0) {
			if (mkdir(buf) < 0) {
				return(-1);
			}
		} else {
			close(fd);
		}
		if (p) {
			*p++ = '/';
		}
	} while(p);

	return(0);
}
@


1.4
log
@Fix up /env initialization.
@
text
@d20 1
a20 1
getenv(char *name)
d70 1
a70 1
setenv(char *name, char *val)
@


1.3
log
@Add a setenv init routine.
@
text
@d10 2
a13 1
extern int wstat();
d115 1
a115 1
 * Usually called after a fork to establish a new base and
d122 2
d126 1
a126 2
	 * If we have a new base, move down to it creating
	 * path elements as needed.
d128 20
a148 6
		char *p, *q;

		/*
		 * Trim leading '/'s, add prefix /env and suffix
		 * "#".
		 */
d153 11
d165 4
a168 5
		/*
		 * Skip first two slashes (from above; they should
		 * definitely exist)
		 */
		p = strchr(buf, '/');
d170 6
a175 9
		++p;

		/*
		 * mkdir successive elements
		 */
		do {
			p = strchr(p, '/');
			if (p) {
				*p = '\0';
d177 7
a183 13
			if ((fd = open(buf, O_READ)) < 0) {
				if (mkdir(buf) < 0) {
					return(-1);
				}
			} else {
				close(fd);
			}
			if (p) {
				*p++ = '/';
			}
		} while(p);
		return(0);
	}
d185 1
a185 5
	/*
	 * Otherwise use wstat() to move to our own environment node
	 */
	fd = open("/env", O_READ);
	return(wstat(__fd_port(fd), "fork"));
@


1.2
log
@Add setenv
@
text
@d12 1
d108 65
@


1.1
log
@Initial revision
@
text
@d63 45
@
