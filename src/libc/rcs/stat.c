head	1.11;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.7
	V1_1:1.7
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.11
date	94.12.21.05.37.27;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.10.23.18.11.13;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.07.10.19.32.46;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.07.10.18.24.44;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.10.17.19.26.07;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.05.07.00.07.35;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.05.03.21.31.37;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.16.19.06.52;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.13.01.31.37;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.26.18.42.18;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.01.33;	author vandys;	state Exp;
branches;
next	;


desc
@A stat(2) emulation, mapping from the ASCII messages the servers
provide.
@


1.11
log
@Prop up the creaky old stat() emulation
@
text
@/*
 * stat.c
 *	All the stat-like functions
 */
#include <sys/param.h>
#include <sys/fs.h>
#include <stat.h>
#include <stdlib.h>
#include <fcntl.h>

/*
 * fieldval()
 *	Given "statbuf" format buffer, extract a field value
 */
static char *
fieldval(char *statbuf, char *field)
{
	int len;
	char *p, *pn;

	len = strlen(field);
	for (p = statbuf; p; p = pn) {
		/*
		 * Carve out next field
		 */
		pn = strchr(p, '\n');
		if (pn) {
			++pn;
		}

		/*
		 * See if we match
		 */
		if (!strncmp(p, field, len)) {
			if (p[len] == '=') {
				return(p+len+1);
			}
		}
	}
	return(0);
}

/*
 * rstat()
 *	Read stat strings from a port
 *
 * If the given string is non-null, only the value of the named field is
 * returned.  Otherwise all fields in "name=value\n" format are returned.
 *
 * The buffer returned is static and overwritten by each call.
 */
char *
rstat(port_t fd, char *field)
{
	struct msg m;
	static char statbuf[MAXSTAT];
	char *p, *q;
	int len;

	/*
	 * Send a stat request
	 */
	m.m_op = FS_STAT|M_READ;
	m.m_buf = statbuf;
	m.m_arg = m.m_buflen = MAXSTAT;
	m.m_arg1 = 0;
	m.m_nseg = 1;
	len = msg_send(fd, &m);
	if (len <= 0) {
		return(0);
	}
	statbuf[len] = '\0';

	/*
	 * No field--return whole thing
	 */
	if (!field) {
		return(statbuf);
	}

	/*
	 * Hunt for named field
	 */
	p = fieldval(statbuf, field);
	if (p == 0) {
		return(0);
	}

	/*
	 * Make it a null-terminated string
	 */
	q = strchr(p, '\n');
	if (q) {
		*q = '\0';
	}
	return(p);
}

/*
 * wstat()
 *	Write a string through the stat function
 */
int
wstat(port_t fd, char *field)
{
	struct msg m;

	m.m_op = FS_WSTAT;
	m.m_buf = field;
	m.m_arg = m.m_buflen = strlen(field);
	m.m_arg1 = 0;
	m.m_nseg = 1;
	return(msg_send(fd, &m));
}

/*
 * field()
 *	Parse a x/y/z field and allow array-type access
 */
static int
field(char *str, int idx)
{
	int x;
	char *p, *eos;

	/*
	 * Walk forward to the n'th field
	 */
	p = str;
	eos = strchr(p, '\n');		/* Don't walk into next field */
	if (eos == 0) {			/* Corrupt */
		return(-1);
	}
	for (x = 0; x < idx; ++x) {
		p = strchr(p, '/');
		if (!p || (p >= eos)) {
			return(-1);
		}
		++p;
	}

	/*
	 * Return value.  Following '/' or '\0' will stop atoi()
	 */
	return(atoi(p));
}

/*
 * modes()
 *	Convert the <sys/fs.h> access bits into the stat.h ones
 */
static int
modes(int v)
{
	int x = 0;

	if (v & ACC_READ) x |= S_IREAD;
	if (v & ACC_WRITE) x |= S_IWRITE;
	if (v & ACC_EXEC) x |= S_IEXEC;
	return(x);
}

/*
 * fstat()
 *	Stat an open file
 */
int
fstat(int fd, struct stat *s)
{
	char *sbuf, *p;
	int mode, aend;
	port_t port;
	dev_t dev;

	if ((port = __fd_port(fd)) < 0) {
		return(-1);
	}
	sbuf = rstat(port, (char *)0);
	if (!sbuf) {
		return(-1);
	}

#define F(stfield, name, defvalue) \
	p = fieldval(sbuf, name); \
	if (p) s->stfield = atoi(p); \
	else s->stfield = defvalue;

	/*
	 * Do translation of simple numeric fields
	 */
	F(st_ino, "inode", 1);
	F(st_nlink, "links", 1);
	F(st_size, "size", 0);
	F(st_atime, "atime", 0);
	F(st_mtime, "mtime", 0);
	F(st_ctime, "ctime", 0);
	F(st_blksize, "block", 512);
	F(st_blocks, "blocks",
	  (s->st_size + s->st_blksize - 1) / s->st_blksize);

	/*
	 * Sort out device/node fields
	 */
	s->st_dev = 0;
	F(st_rdev, "dev", 0);
	dev = s->st_rdev;
	s->st_rdev = makedev(dev, s->st_ino);

	/*
	 * Set UID/GID
	 */
	p = fieldval(sbuf, "owner");
	if (p) {
		s->st_uid = field(p, 0);
		if (s->st_uid == -1) {
			s->st_uid = 0;
		}
		s->st_gid = field(p, 1);
		if (s->st_gid == -1) {
			s->st_gid = 0;
		}
	} else {
		s->st_gid = s->st_uid = 0;
	}

	/*
	 * Decode "type"
	 */
	p = fieldval(sbuf, "type");
	if (!p) {
		mode = S_IFREG;
	} else if (!strncmp(p, "d\n", 2)) {
		mode = S_IFDIR;
	} else if (!strncmp(p, "c\n", 2)) {
		mode = S_IFCHR;
	} else if (!strncmp(p, "b\n", 2) || !strncmp(p, "s\n", 2)) {
		mode = S_IFBLK;
	} else if (!strncmp(p, "fifo\n", 5)) {
		mode = S_IFIFO;
	} else {
		mode = S_IFREG;
	}

	/*
	 * Map the default access fields into "other" - we have a bit of a
	 * problem with the "group" and "user" so we assume that the user
	 * permission is that granted to someone who's ID dominates and
	 * arbitrarily decide that the group corresponds to someone who's
	 * ID matches to the penultimate position.
	 */
	p = fieldval(sbuf, "acc");
	if (p) {
		/*
		 * We need to determine how long the access rights info
		 * really is
		 */
		aend = PERMLEN - 1;
		while((field(p, aend) == -1) && (aend > 0)) {
			aend--;
		}

		mode |= (modes(field(p, 0)) >> 6);
		mode |= ((aend >= 1) ? ((modes(field(p, aend - 1))) >> 3) : 0)
			| ((mode & 0007) << 3);
		mode |= ((aend >= 0) ? modes(field(p, aend)) : 0)
			| ((mode & 0070) << 3);
	} else {
		mode |= ((S_IREAD|S_IWRITE));
	}
	s->st_mode = mode;
	return(0);
}

/*
 * stat()
 *	Open file and get its fstat() information
 */
int
stat(const char *f, struct stat *s)
{
	int fd, x;

	fd = open(f, O_READ);
	if (fd < 0) {
		return(-1);
	}
	x = fstat(fd, s);
	close(fd);
	return(x);
}

/*
 * isatty()
 *	Tell if given port talks to a TTY-like device
 */
int
isatty(int fd)
{
	port_t port;
	char *p;

	if ((port = __fd_port(fd)) < 0) {
		return(0);
	}
	p = rstat(port, "type");
	if (p && (p[0] == 'c') && (p[1] == '\0')) {
		return(1);
	}
	return(0);
}
@


1.10
log
@Fix stat() emulation, add more compat macros
@
text
@d8 1
a8 1
#include <std.h>
d131 2
a132 2
	if (eos == 0) {	/* Corrupt */
		return(0);
d137 1
a137 1
			return(0);
d171 1
a171 1
	int mode;
d215 3
d219 3
d245 5
a249 1
	 * Map the default access fields into "other"
d253 14
a266 3
		mode |= modes(field(p, 0) >> 6);
		mode |= (modes(field(p, 1)) >> 3) | ((mode & 0007) << 3);
		mode |= modes(field(p, 2)) | ((mode & 0070) << 3);
@


1.9
log
@Add blocks value for # blocks used by a file
@
text
@d173 1
a190 1
	F(st_dev, "dev", 0);
a192 1
	F(st_rdev, "dev", 0);
d197 11
a207 2
	F(st_blksize, "block", NBPG);
	F(st_blocks, "blocks", NBPG);
d230 1
a230 1
	} else if (!strncmp(p, "b\n", 2)) {
d241 1
a241 1
	p = fieldval(sbuf, "perm");
d243 3
a245 3
		mode |= modes(field(p, 0));
		mode |= modes(field(p, 1)) << 3;
		mode |= modes(field(p, 2)) << 6;
d247 1
a247 1
		mode |= ((S_IREAD|S_IWRITE) << 6);
@


1.8
log
@Add rmdir(), add some const definitions
@
text
@d199 1
@


1.7
log
@Get rid of __isatty(); caller needed to check for two "type"
return values now.
@
text
@d103 1
d120 1
a120 1
static
d152 1
a152 1
static
d167 1
d248 2
a249 1
stat(char *f, struct stat *s)
d266 1
@


1.6
log
@Fields are separated with newlines, not \0.  Need to use
strncmp to check for field value.
@
text
@a259 15
 * __isatty()
 *	Internal version which works on a port, not a fd
 */
__isatty(port_t port)
{
	char *p;

	p = rstat(port, "type");
	if (p && (p[0] == 'c') && (p[1] == '\0')) {
		return(1);
	}
	return(0);
}

/*
d266 1
d271 5
a275 1
	return(__isatty(port));
@


1.5
log
@Forgot to null-terminate
@
text
@d123 1
a123 1
	char *p;
d129 4
d135 1
a135 1
		if (!p) {
d203 2
a204 2
		s->st_gid = field(p, 0);
		s->st_uid = field(p, 1);
d215 1
a215 1
	} else if (!strcmp(p, "d")) {
d217 1
a217 1
	} else if (!strcmp(p, "c")) {
d219 1
a219 1
	} else if (!strcmp(p, "b")) {
d221 1
a221 1
	} else if (!strcmp(p, "fifo")) {
a234 1
		s->st_mode = mode;
d236 1
a236 1
		s->st_mode |= ((S_IREAD|S_IWRITE) << 6);
d238 1
@


1.4
log
@Need to null-terminate in rstat() of a field now that
fieldval doesn't do it.  fieldval() was changed so that
the same stat string could be used for multiple lookups;
I didn't realize how this impacted rstat() until now.
@
text
@d58 1
d68 2
a69 1
	if (msg_send(fd, &m) <= 0) {
d72 1
@


1.3
log
@Don't modify string; breaks successive calls
@
text
@d57 1
d81 13
a93 1
	return(fieldval(statbuf, field));
@


1.2
log
@Make fstat() more forgiving
@
text
@d28 1
a28 1
			*pn++ = '\0';
@


1.1
log
@Initial revision
@
text
@d182 5
a186 2
	if (!p) {
		return(-1);
a187 2
	s->st_gid = field(p, 0);
	s->st_uid = field(p, 1);
d194 2
a195 3
		return(-1);
	}
	if (!strcmp(p, "d")) {
d204 1
a204 1
		mode = 0;
d211 7
a217 2
	if (!p) {
		return(-1);
a218 6
	mode |= modes(field(p, 0));
	mode |= modes(field(p, 1)) << 3;
	mode |= modes(field(p, 2)) << 3;
	mode |= modes(field(p, 3)) << 3;
	mode |= modes(field(p, 4)) << 3;
	s->st_mode = mode;
@
