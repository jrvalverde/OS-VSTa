head	1.21;
access;
symbols
	V1_3_1:1.13
	V1_3:1.12
	V1_2:1.11
	V1_1:1.10
	V1_0:1.9;
locks; strict;
comment	@ * @;


1.21
date	95.02.04.16.44.05;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	95.02.02.04.37.24;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	94.10.13.14.17.59;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	94.09.23.20.37.15;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.07.10.18.24.44;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.06.07.23.01.31;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.05.24.16.57.16;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.05.17.05.01.01;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.04.26.21.37.02;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.04.02.01.57.10;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.11.25.20.20.51;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.08.29.22.54.28;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.05.21.17.42.41;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.04.20.21.25.14;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.03.25.21.28.57;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.24.19.09.31;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.16.22.23.47;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.10.18.43.21;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.05.23.31.18;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.15.08.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.00.08;	author vandys;	state Exp;
branches;
next	;


desc
@Code for doing mount table scans and pathname lookup.
@


1.21
log
@Remove old debug printf
@
text
@/*
 * open.c
 *	Our fancy routines to open a file
 *
 * Unlike UNIX, and like Plan9, we walk our way down the directory
 * tree manually.
 */
#include <sys/fs.h>
#include <std.h>
#include <mnttab.h>
#include <fcntl.h>
#include <alloc.h>
#include <pwd.h>

#define MAXLINK (16)	/* Max levels of symlink to follow */
#define MAXSYMLEN (128)	/*  ...max length of one element */

static char default_wd[] = "/";

char *__cwd =		/* Path to current directory */
	default_wd;
extern struct mnttab	/* List of mounted paths */
	*__mnttab;
extern int __nmnttab;	/*  ...# elements in __mnttab */

/*
 * mapmode()
 *	Convert to <sys/fs.h> format of modes
 */
static int
mapmode(int mode)
{
	int m = 0;

	if (mode & O_READ) m |= ACC_READ;
	if (mode & O_WRITE) m |= ACC_WRITE;
	if (mode & O_CREAT) m |= ACC_CREATE;
	if (mode & O_DIR) m |= ACC_DIR;
	return(m);
}

/*
 * __dotdot()
 *	Remove ".."'s from path
 *
 * Done here because our file servers don't have our context
 * for interpretation.  Assumes path is in absolute format, with
 * a leading '/'.
 *
 * Also takes care of "."'s.
 */
int
__dotdot(char *path)
{
	int nelem = 0, x, had_dot = 0;
	char **elems = 0, *p = path;

	/*
	 * Map elements into vector
	 */
	while (p = strchr(p, '/')) {
		*p++ = '\0';
		nelem += 1;
		elems = realloc(elems, nelem*sizeof(char *));
		if (!elems) {
			return(1);
		}
		elems[nelem-1] = p;
	}

	/*
	 * Find ".."'s, and wipe them out
	 */
	x = 0;
	while (x < nelem) {
		/*
		 * Wipe out "." and empty ("//") elements
		 */
		if (!elems[x][0] || !strcmp(elems[x], ".")) {
			had_dot = 1;
			nelem -= 1;
			bcopy(elems+x+1, elems+x,
				(nelem-x)*sizeof(char *));
			continue;
		}

		/*
		 * If not "..", keep going
		 */
		if (strcmp(elems[x], "..")) {
			x += 1;
			continue;
		}

		/*
		 * Can't back up before root.  Just ignore.
		 */
		had_dot = 1;
		if (x == 0) {
			nelem -= 1;
			bcopy(elems+1, elems, nelem*sizeof(char *));
			continue;
		}

		/*
		 * Wipe out the "..", and the element before it
		 */
		nelem -= 2;
		x -= 1;
		bcopy(elems+x+2, elems+x, (nelem-x)*sizeof(char *));
	}

	/*
	 * No ".."'s--just put slashes back in place and return
	 */
	if (!had_dot) {
		for (x = 0; x < nelem; ++x) {
			elems[x][-1] = '/';
		}
		free(elems);
		return(0);
	}

	/*
	 * If no elements, always provide "/"
	 */
	if (nelem < 1) {
		*path++ = '/';
	} else {
		/*
		 * Rebuild path.  path[0] is already '\0' because of
		 * the requirement that all paths to __dotdot() be
		 * absolute and thus the first '/' was written to \0
		 * by the first loop of this routine.
		 */
		for (x = 0; x < nelem; ++x) {
			int len;

			*path++ = '/';
			len = strlen(elems[x]);
			bcopy(elems[x], path, len);
			path += len;
		}
	}
	*path = '\0';

	/*
	 * All done
	 */
	free(elems);
	return(0);
}

/*
 * follow_symlink()
 *	Try to extract a symlink contents and build a new path
 *
 * Returns a new malloc()'ed  string containing the mapped path
 */
static char *
follow_symlink(port_t port, char *file, char *p)
{
	struct msg m;
	port_t port2;
	int x, len;
	char *newpath, *lenstr;

	/*
	 * Walk down a copy into the file so we don't lose
	 * our place in the path.
	 */
	port2 = clone(port);
	if (port2 < 0) {
		return(0);
	}
	m.m_op = FS_OPEN;
	m.m_buf = file;
	m.m_buflen = strlen(file)+1;
	m.m_nseg = 1;
	m.m_arg = ACC_READ | ACC_SYM;
	m.m_arg1 = 0;
	x = msg_send(port2, &m);

	/*
	 * If we can't get it, bail
	 */
	if (x < 0) {
		msg_disconnect(port2);
		return(0);
	}

	/*
	 * Calculate length, get a buffer to hold new path
	 */
	lenstr = rstat(port2, "size");
	if (lenstr == 0) {
		msg_disconnect(port2);
		return(0);
	}
	len = atoi(lenstr);
	if (len > MAXSYMLEN) {
		msg_disconnect(port2);
		return(0);
	}
	newpath = malloc(len + (p ? (strlen(p+1)) : 0) + 1);
	if (newpath == 0) {
		msg_disconnect(port2);
		return(0);
	}

	/*
	 * Read the contents
	 */
	m.m_op = FS_READ | M_READ;
	m.m_nseg = 1;
	m.m_buf = newpath;
	m.m_arg = m.m_buflen = len;
	m.m_arg1 = 0;
	x = msg_send(port2, &m);
	if (x < 0) {
		free(newpath);
		msg_disconnect(port2);
		return(0);
	}

	/*
	 * Tack on the remainder of the path
	 */
	if (p) {
		sprintf(newpath+len, "/%s", p+1);
	} else {
		newpath[len] = '\0';
	}

	/*
	 * There's your new path
	 */
	return(newpath);
}

/*
 * try_open()
 *	Given a root point and a path, try to walk into the mount
 *
 * Returns 1 on error, 0 on success.
 */
static int
try_open(port_t newfile, char *file, int mask, int mode)
{
	char *p;
	struct msg m;
	int x, nlink = 0;

	/*
	 * The mount point itself is a special case
	 */
	if (file[0] == '\0') {
		return(0);
	}

	/*
	 * Otherwise walk each element in the path
	 */
	do {
		char *tmp;

		/*
		 * Find the next '/', or end of string
		 */
		while (*file == '/') {
			++file;
		}
		p = strchr(file, '/');
		if (p) {
			*p = '\0';
		}

		/*
		 * Map element to a getenv() of it if it  has
		 * a "@@" prefix.
		 */
		tmp = 0;
		if (*file == '@@') {
			tmp = getenv(file+1);
			if (tmp) {
				file = tmp;
			}
		}

		/*
		 * Try to walk our file down to the new node
		 */
		m.m_op = FS_OPEN;
		m.m_buf = file;
		m.m_buflen = strlen(file)+1;
		m.m_nseg = 1;
		m.m_arg = p ? ACC_EXEC : mode;
		m.m_arg1 = p ? 0 : mask;
		x = msg_send(newfile, &m);
		if (tmp) {
			free(tmp);	/* Free env storage if any */
		}

		/*
		 * If we encounter a symlink, see about following it
		 */
		if ((x < 0) && !strcmp(strerror(), ESYMLINK)) {
			/*
			 * Cap number of levels of symlink we'll follow
			 */
			if (nlink++ >= MAXLINK) {
				__seterr(ELOOP);
				return(1);
			}

			/*
			 * Pull in contents of symlink, make that
			 * our new path to lookup
			 */
			tmp = follow_symlink(newfile, file, p);
			if (tmp) {
				file = alloca(strlen(tmp)+1);
				strcpy(file, tmp);
				free(tmp);
				continue;
			}
		}

		if (p) {
			*p++ = '/';	/* Restore path seperator */
		}
		if (x < 0) {		/* Return error if any */
			return(1);
		}

		/*
		 * Advance to next element
		 */
		file = p;
	} while (file);
	return(0);
}

/*
 * do_home()
 *	Rewrite ~ to $HOME, and ~foo to foo's home dir
 *
 * Result is returned in a malloc()'ed buffer on success, 0 on failure
 */
static char *
do_home(char *in)
{
	char *p, *q, *name, *name_end;

	/*
	 * Find end of element, either first '/' or end of string.
	 * If '/', create private copy of name, null-terminated.
	 */
	p = strchr(in, '/');
	if (p == 0) {
		name = in;
		name_end = in + strlen(in);
	} else {
		name = alloca((p - in) + 1);
		bcopy(in, name, p - in);
		name[p - in] = '\0';
		name_end = p;
	}

	if (!strcmp(name, "~")) {
		static char *home;

		/*
		 * ~ -> $HOME.  Cache it for speed on reuse.
		 */
		if (home == 0) {
			home = getenv("HOME");
		}
		p = home;
	} else {
		struct passwd *pw;

		/*
		 * ~name -> $HOME for "name"
		 */


		/*
		 * Look it up in the password database
		 */
		pw = getpwnam(name+1);
		if (pw) {
			p = pw->pw_dir;
		} else {
			p = 0;
		}
	}

	/*
	 * If didn't find ~name's home or $HOME, return 0
	 */
	if (p == 0) {
		return(0);
	}

	/*
	 * malloc() result, assemble our replacement part and
	 * rest of original path
	 */
	q = malloc(strlen(p) + strlen(name_end) + 1);
	if (q == 0) {
		return(0);
	}
	sprintf(q, "%s%s", p, name_end);
	return(q);
}

/*
 * open()
 *	Open a file
 */
int
open(const char *file, int mode, ...)
{
	int x, len, mask;
	port_t newfile;
	char buf[MAXPATH], *p, *home_buf;
	struct mnttab *mt, *match = 0;
	struct mntent *me;

	/*
	 * Before first mount, can't open anything!
	 */
	if (__mnttab == 0) {
		return(__seterr(ESRCH));
	}

	/*
	 * If O_CREAT, get mask.
	 * XXX this isn't very portable, but I HATE the varargs interface.
	 */
	if (mode & O_CREAT) {
		mask = *((&mode)+1);
	} else {
		mask = 0;
	}

	/*
	 * Map to <sys/fs.h> format for access bits
	 */
	mode = mapmode(mode);

	/*
	 * Rewrite ~
	 */
	if (file[0] == '~') {
		home_buf = do_home((char *)file);
		if (home_buf) {
			file = home_buf;
		}
	} else {
		home_buf = 0;
	}

	/*
	 * See where to start.  We always have to copy the string
	 * because "__dotdot" is going to write it, and the supplied
	 * string might be const, and thus perhaps not writable.
	 */
	if (file[0] == '/') {
		strcpy(buf, file);
	} else {
		sprintf(buf, "%s/%s", __cwd, file);
	}
	p = buf;

	/*
	 * Free $HOME processing buffer now that we've used it
	 */
	if (home_buf) {
		free(home_buf);
	}

	/*
	 * Remove ".."s
	 */
	if (__dotdot(p)) {
		return(-1);
	}

	/*
	 * Scan for longest match in mount table
	 */
	len = 0;
	for (x = 0; x < __nmnttab; ++x) {
		char *q, *r;

		/*
		 * Scan strings until end or mismatch
		 */
		mt = &__mnttab[x];
		for (q = mt->m_name, r = p; *q && *r; ++q, ++r) {
			if (*q != *r) {
				break;
			}
		}

		/*
		 * Exact match--end now
		 */
		if (!r[0] && !q[0]) {
			len = strlen(mt->m_name);
			match = mt;
			break;
		}

		/*
		 * Mount path ended first.  If this is the longest
		 * match so far, record this as the current prefix
		 * directory.
		 */
		if (r[0] && !q[0]) {
			/*
			 * Don't allow, say, /vsta to match against /v in
			 * the mount table.  Require that the mount
			 * table entry /xyz match against /xyz/...
			 * Note special case for root mounts, where mount
			 * name ends with /, all other mount points end
			 * in non-/.
			 */
			if (((q[-1] == '/') && (r[-1] == '/')) ||
				((q[-1] != '/') && (r[0] == '/')))
			{
				if ((q - mt->m_name) > len) {
					len = q - mt->m_name;
					match = mt;
				}
				continue;
			}

		} else if (*q != *r) {
			/*
			 * Mismatch--ignore entry and continue scan
			 */
			continue;
		}

		/*
		 * Else our target string ended first--ignore
		 * this element, since it isn't for us.
		 */
	}

	/*
	 * We now have our starting point.  Advance our target
	 * string to ignore the leading prefix.
	 */
	p += len;

	/*
	 * No matches--no hope of an open() succeeding
	 */
	if ((match == 0) || (match->m_entries == 0)) {
		return(__seterr(ESRCH));
	}

	/*
	 * Try each mntent under the chosen mnttab slot
	 */
	for (me = match->m_entries; me; me = me->m_next) {
		newfile = clone(me->m_port);
		if (try_open(newfile, p, mask, mode) == 0) {
			x = __fd_alloc(newfile);
			if (x < 0) {
				msg_disconnect(newfile);
				return(__seterr(ENOMEM));
			}
			return(x);
		}
		msg_disconnect(newfile);
	}

	/*
	 * The interaction with the port server will have set
	 * the error string already.
	 */
	return(-1);
}

/*
 * chdir()
 *	Change current directory
 */
int
chdir(const char *newdir)
{
	char buf[MAXPATH], *p;
	int fd;

	/*
	 * Get copy, flatten ".."'s out
	 */
	if (newdir[0] == '/') {
		strcpy(buf, newdir);
	} else {
		sprintf(buf, "%s/%s", __cwd, newdir);
	}
	__dotdot(buf);

#ifdef XXX
	/*
	 * Try to open it.
	 */
	fd = open(buf, O_READ);
	if (fd < 0) {
		return(-1);
	}

	/*
	 * Make sure it's a directory-like substance
	 */
	{
		extern char *rstat();

		p = rstat(__fd_port(fd), "type");
	}
	if (!p || strcmp(p, "d")) {
		return(-1);
	}
	close(fd);
#endif

	/*
	 * Looks OK.  Move to this dir.
	 */
	p = strdup(buf);
	if (!p) {
		return(__seterr(ENOMEM));
	}
	if (__cwd != default_wd) {
		free(__cwd);
	}
	__cwd = p;
	return(0);
}

/*
 * mkdir()
 *	Create a directory
 */
int
mkdir(const char *dir)
{
	int fd;

	fd = open(dir, O_CREAT|O_DIR);
	if (fd > 0) {
		close(fd);
		return(0);
	}
	return(-1);
}

/*
 * __cwd_size()
 *	Tell how many bytes to save cwd state
 */
long
__cwd_size(void)
{
	/*
	 * Length of string, null byte, and a leading one-byte count.
	 * If cwd is ever > 255, this would have to change.
	 */
	return(strlen(__cwd)+2);
}

/*
 * __cwd_save()
 *	Save cwd into byte stream
 */
void
__cwd_save(char *p)
{
	*p++ = strlen(__cwd)+1;
	strcpy(p, __cwd);
}

/*
 * __cwd_restore()
 *	Restore cwd from byte stream, return pointer to next data
 */
char *
__cwd_restore(char *p)
{
	int len;

	len = ((*p++) & 0xFF);
	__cwd = malloc(len);
	if (__cwd == 0) {
		abort();
	}
	bcopy(p, __cwd, len);
	return(p+len);
}

/*
 * getcwd()
 *	Get current working directory
 */
char *
getcwd(char *buf, int len)
{
	if (strlen(__cwd) >= len) {
		return(0);
	}
	strcpy(buf, __cwd);
	return(buf);
}

/*
 * unlink()
 *	Move down to dir containing entry, and try to remove it
 */
int
unlink(const char *path)
{
	int fd, x, tries;
	char *dir, *file, buf[MAXPATH];
	struct msg m;

	/*
	 * Get writable copy of string, flatten out ".."'s and
	 * parse into a directory and filename.
	 */
	strcpy(buf, path);
	__dotdot(buf);
	file = strrchr(buf, '/');
	if (file) {
		dir = buf;
		*file++ = '\0';
	} else {
		dir = __cwd;
		file = buf;
	}

	/*
	 * Get access to the directory
	 */
	fd = open(dir, O_DIR|O_READ);
	if (fd < 0) {
		return(-1);
	}

	/*
	 * Ask to remove the filename within this dir.  EAGAIN can
	 * come back it we race with the a.out caching.
	 */
	for (tries = 0; tries < 3; ++tries) {
		m.m_op = FS_REMOVE;
		m.m_buf = file;
		m.m_buflen = strlen(file)+1;
		m.m_nseg = 1;
		m.m_arg = m.m_arg1 = 0;
		x = msg_send(__fd_port(fd), &m);
		if ((x >= 0) || strcmp(strerror(), EAGAIN)) {
			break;
		}
		__msleep(100);
	}

	/*
	 * Clean up and return results
	 */
	close(fd);
	return(x);
}

/*
 * creat()
 *	Create a file - basically a specialised version of open()
 */
int
creat(const char *path, int mode)
{
	return(open(path, (O_WRONLY | O_CREAT | O_TRUNC), mode));
}

/*
 * rmdir()
 *	Remove a directory
 */
int
rmdir(const char *olddir)
{
	int fd;
	char *p;

	/*
	 * Open entry
	 */
	fd = open(olddir, O_READ, 0);
	if (fd < 0) {
		return(-1);
	}

	/*
	 * See if it's a dir
	 */
	p = rstat(__fd_port(fd), "type");
	close(fd);
	if (!p || strcmp(p, "d")) {
		__seterr(ENOTDIR);
		return(-1);
	}
	return(unlink(olddir));
}
@


1.20
log
@Add ~ and ~name handling to path processing
@
text
@a414 1
	printf("Ret: %s\n", q);
@


1.19
log
@Add some const declarations
@
text
@d13 1
d345 75
d428 1
a428 1
	char buf[MAXPATH], *p;
d455 12
d477 7
@


1.18
log
@Add symlink and environment link support
@
text
@d558 1
a558 1
mkdir(char *dir)
d690 1
a690 1
creat(char *path, int mode)
@


1.17
log
@Add rmdir(), add some const definitions
@
text
@d12 1
d14 3
d154 87
d251 1
a251 1
	int x;
d264 2
d278 12
d299 29
@


1.16
log
@Remove extraneous msg_disconnect(); it overwrote the ENOENT
error from the true file operation
@
text
@d25 1
a25 1
static
d47 1
d155 1
a155 1
static
d213 2
a214 1
open(char *file, int mode, ...)
d423 1
d497 2
a498 1
unlink(char *path)
d555 12
a566 1
int creat(char *path, int mode)
d568 21
a588 1
	return open(path, (O_WRONLY | O_CREAT | O_TRUNC), mode);
@


1.15
log
@Fix /v vs. /vsta mount point matching
@
text
@a196 1
			msg_disconnect(newfile);
@


1.14
log
@Don't allow /v to match a /vsta patch search
@
text
@d298 3
d302 3
a304 1
			if (q[0] == '/') {
@


1.13
log
@Leave path buffer unchanged so multiple servers under a union
dir can try the path
@
text
@d294 11
a304 3
			if ((q - mt->m_name) > len) {
				len = q - mt->m_name;
				match = mt;
a305 2
			continue;
		}
d307 4
a310 4
		/*
		 * Mismatch--ignore entry and continue scan
		 */
		if (*q != *r) {
@


1.12
log
@Add creat()
@
text
@d159 1
d180 1
a180 1
			*p++ = '\0';
d192 5
a196 1
		if (msg_send(newfile, &m) < 0) {
@


1.11
log
@Completely bogus close of fd instead of disconnect from port
@
text
@d531 9
@


1.10
log
@Add const modifier and explicit type
@
text
@d192 1
a192 1
			close(newfile);
@


1.9
log
@Handle various tedium of paths with "//" and empty paths
@
text
@d349 2
a350 1
chdir(char *newdir)
@


1.8
log
@Add retry on EAGAIN when unlink()'ing a file.  Allows file server
time to unhash/free refs to cached a.out
@
text
@d56 1
a56 1
		*p = '\0';
d62 1
a62 1
		p = elems[nelem-1] = p+1;
d71 1
a71 1
		 * Wipe out "." elements
d73 1
a73 1
		if (!strcmp(elems[x], ".")) {
d119 1
a119 3
	 * Rebuild path.  path[0] is already '\0' because of the requirement
	 * that all paths to __dotdot() be absolute and thus the first '/' was
	 * written to \0 by the first loop of this routine.
d121 11
a131 2
	for (x = 0; x < nelem; ++x) {
		int len;
d133 5
a137 4
		*path++ = '/';
		len = strlen(elems[x]);
		bcopy(elems[x], path, len);
		path += len;
@


1.7
log
@Make dotdot() available globally; rename it to conform to
POSIX requirements
@
text
@d472 1
a472 1
	int fd, x;
d500 2
a501 1
	 * Ask to remove the filename within this dir
d503 12
a514 6
	m.m_op = FS_REMOVE;
	m.m_buf = file;
	m.m_buflen = strlen(file)+1;
	m.m_nseg = 1;
	m.m_arg = m.m_arg1 = 0;
	x = msg_send(__fd_port(fd), &m);
@


1.6
log
@Set local error string on Clib-detected errors
@
text
@d38 1
a38 1
 * dotdot()
d47 1
a47 2
static
dotdot(char *path)
d120 1
a120 1
	 * that all paths to dotdot() be absolute and thus the first '/' was
d232 1
a232 1
	 * because "dotdot" is going to write it, and the supplied
d245 1
a245 1
	if (dotdot(p)) {
d354 1
a354 1
	dotdot(buf);
d481 1
a481 1
	dotdot(buf);
@


1.5
log
@Add unlink()
@
text
@d213 1
a213 1
		return(-1);
d311 2
a312 2
	if (match == 0) {
		return(-1);
d324 1
a324 1
				return(-1);
d330 5
d385 1
a385 1
		return(-1);
@


1.4
log
@Add getcwd()
@
text
@d461 50
@


1.3
log
@Add passing of CWD through exec()
@
text
@d447 14
@


1.2
log
@Fix mount table handling, forget about string ordering, and fix
some mistakes made during conversion of file descriptors to
ports.
@
text
@d404 43
@


1.1
log
@Initial revision
@
text
@d319 1
a319 1
		newfile = clone(me->m_fd);
@
