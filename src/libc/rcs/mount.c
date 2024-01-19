head	1.9;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.7
	V1_1:1.7
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.9
date	94.09.23.20.37.41;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.02.28.19.14.32;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.03.24.17.44.30;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.24.00.36.20;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.23.18.56.35;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.20.00.22.58;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.26.18.42.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.15.08.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.00.02;	author vandys;	state Exp;
branches;
next	;


desc
@Mount table handling
@


1.9
log
@Create procedural interfaces to all global C library data
@
text
@/*
 * mount.c
 *	Routines for manipulating mount table
 */
#include <mnttab.h>
#include <std.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/ports.h>

struct mnttab *__mnttab;
int __nmnttab = 0;

/*
 * __get_mntinfo()
 *	Provide procedural interface to access mount data structures
 */
void
__get_mntinfo(int *countp, struct mnttab **mntp)
{
	*countp = __nmnttab;
	*mntp = __mnttab;
}

/*
 * mountport()
 *	Like mount(), but mount on given port
 */
mountport(char *point, port_t port)
{
	int x;
	struct mnttab *mt;
	struct mntent *me;

	/*
	 * Scan mount table for this point
	 */
	for (x = 0; x < __nmnttab; ++x) {
		/*
		 * Compare to entry
		 */
		if (!strcmp(point, __mnttab[x].m_name)) {
			/*
			 * On exact match, end loop
			 */
			break;
		}
	}

	/*
	 * Get memory for mntent
	 */
	me = malloc(sizeof(struct mntent));
	if (!me) {
		return(-1);
	}
	me->m_port = port;

	/*
	 * If needed, insert new mnttab slot
	 */
	if (x >= __nmnttab) {
		/*
		 * Grow mount table
		 */
		mt = realloc(__mnttab, (__nmnttab+1)*sizeof(struct mnttab));
		if (!mt) {
			free(me);
			return(-1);
		}
		__mnttab = mt;

		/*
		 * Add new mount slot to end of table
		 */
		mt += __nmnttab;
		__nmnttab += 1;

		/*
		 * Fill in slot with name
		 */
		mt->m_name = strdup(point);
		if (mt->m_name == 0) {
			/*
			 * What a really great time to run out of memory.
			 * Leave the mount table at its new size; malloc
			 * can handle this next time around.
			 */
			__nmnttab -= 1;
			free(me);
			return(-1);
		}
		mt->m_len = strlen(mt->m_name);
		mt->m_entries = 0;
	} else {
		mt = &__mnttab[x];
	}

	/*
	 * Add our entry to the slot
	 */
	me->m_next = mt->m_entries;
	mt->m_entries = me;

	return(0);
}

/*
 * mount()
 *	Mount port from the given lookup onto the named point
 */
mount(char *point, char *what)
{
	int fd;

	/*
	 * Do initial open.
	 */
	if ((fd = open(what, O_READ)) < 0) {
		return(-1);
	}

	/*
	 * Let mountport do the rest
	 */
	if (mountport(point, __fd_port(fd)) < 0) {
		close(fd);
		return(-1);
	}
	return(fd);
}

/*
 * umount()
 *	Delete given entry from mount list
 *
 * If fd is -1, all mounts at the given point are removed.  Otherwise
 * only the mount with the given port (fd) will be removed.  XXX we
 * need to hunt down the FDL entry as well.
 */
umount(char *point, port_t port)
{
	int x;
	struct mnttab *mt;
	struct mntent *me, *men;

	/*
	 * Scan mount table for this string
	 */
	for (x = 0; x < __nmnttab; ++x) {
		mt = &__mnttab[x];
		if (!strcmp(point, mt->m_name)) {
			break;
		}
	}

	/*
	 * Not found--fail
	 */
	if (x >= __nmnttab) {
		return(-1);
	}

	/*
	 * If "port" given, look for particular slot
	 */
	if (port >= 0) {
		struct mntent **mp;

		mp = &mt->m_entries;
		for (me = mt->m_entries; me; me = me->m_next) {
			/*
			 * When spotted, patch out of list.
			 */
			if (me->m_port == port) {
				*mp = me->m_next;
				free(me);

				/*
				 * If mnttab slot now empty, drop down
				 * below to clean it up.
				 */
				if (mt->m_entries == 0) {
					break;
				}
				return(0);
			}

			/*
			 * Otherwise advance our back-patch pointer
			 */
			mp = &me->m_next;
		}

		/*
		 * Never found it
		 */
		return(-1);
	}

	/*
	 * Dump all entries, remove mnttab slot
	 */
	for (me = mt->m_entries; me; me = men) {
		men = me->m_next;
		msg_disconnect(me->m_port);
		free(me);
	}
	free(mt->m_name);
	__nmnttab -= 1;
	bcopy(mt+1, mt,
		(__nmnttab - (mt-__mnttab)) * sizeof(struct mnttab));
	return(0);
}

/*
 * __mount_size()
 *	Tell how big the save state of the mount table would be
 */
ulong
__mount_size(void)
{
	ulong len;
	uint x;
	struct mnttab *mt;

	/*
	 * Count of mnttab slots
	 */
	len = sizeof(ulong);

	/*
	 * For each mount table slot
	 */
	for (x = 0; x < __nmnttab; ++x) {
		struct mntent *me;

		mt = &__mnttab[x];
		len += (strlen(mt->m_name)+1);
		len += sizeof(uint);	/* Count of mntent's here */
		for (me = mt->m_entries; me; me = me->m_next) {
			len += sizeof(port_t);
		}
	}
	return(len);
}

/*
 * __mount_save()
 *	Save mount table state into byte array
 */
void
__mount_save(char *p)
{
	uint x, l;
	struct mnttab *mt;

	/*
	 * Count of mnttab slots
	 */
	*(ulong *)p = __nmnttab;
	p += sizeof(ulong);

	/*
	 * For each mount table slot
	 */
	for (x = 0; x < __nmnttab; ++x) {
		struct mntent *me;
		ulong *lp;

		mt = &__mnttab[x];

		/*
		 * Copy in string
		 */
		l = strlen(mt->m_name)+1;
		bcopy(mt->m_name, p, l);
		p += l;

		/*
		 * Record where mntent count will go
		 */
		lp = (ulong *)p;
		p += sizeof(ulong);
		l = 0;

		/*
		 * Scan mntent's, storing port # in place
		 */
		for (me = mt->m_entries; me; me = me->m_next) {
			*(port_t *)p = me->m_port;
			p += sizeof(port_t);
			l += 1;
		}

		/*
		 * Back-patch mntent count now that we know
		 */
		*lp = l;
	}
}

/*
 * __mount_restore()
 *	Restore mount state from byte array
 */
char *
__mount_restore(char *p)
{
	ulong x, len;
	uint l;
	struct mnttab *mt;

	/*
	 * Count of mnttab slots
	 */
	__nmnttab = len = *(ulong *)p;
	p += sizeof(ulong);

	/*
	 * Get mnttab
	 */
	__mnttab = malloc(sizeof(struct mnttab) * len);
	if (__mnttab == 0) {
		abort();
	}

	/*
	 * For each mount table slot
	 */
	for (x = 0; x < len; ++x) {
		struct mntent *me, **mp;
		uint y;

		mt = &__mnttab[x];

		/*
		 * Copy in string
		 */
		l = strlen(p)+1;
		mt->m_name = malloc(l);
		if (mt->m_name == 0) {
			abort();
		}
		bcopy(p, mt->m_name, l);
		p += l;

		/*
		 * Get mntent count
		 */
		l = *(ulong *)p;
		p += sizeof(ulong);

		/*
		 * Generate mntent's
		 */
		mp = &mt->m_entries;
		for (y = 0; y < l; ++y) {
			/*
			 * Get next mntent
			 */
			me = malloc(sizeof(struct mntent));
			if (me == 0) {
				abort();
			}

			/*
			 * Tack onto linked list
			 */
			me->m_port = *(port_t *)p;
			*mp = me;
			mp = &me->m_next;
			p += sizeof(port_t);
		}

		/*
		 * Terminate with null
		 */
		*mp = 0;
	}
	return(p);
}

/*
 * mount_init()
 *	Read initial mounts from table, put in our mount table
 */
mount_init(char *fstab)
{
	FILE *fp;
	char *r, buf[80], *point;
	port_t p;
	extern port_t path_open(char *, int);

	if ((fp = fopen(fstab, "r")) == NULL) {
		return(-1);
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
			printf("mount: mangled line '%s'\n", buf);
			continue;
		}
		*point++ = '\0';

		/*
		 * Open access down into either namer path or
		 * namer path plus subdirs.
		 */
		p = path_open(buf, ACC_READ);
		if (p < 0) {
			printf("mount: can't connect to: %s\n", buf);
		}

		/*
		 * Mount port in its place
		 */
		mountport(point, p);
	}
	fclose(fp);
	return(0);
}

/*
 * mount_port()
 *	Return port for first mntent in given slot
 */
port_t
mount_port(char *point)
{
	int x;
	struct mnttab *mt;

	for (x = 0; x < __nmnttab; ++x) {
		mt = &__mnttab[x];
		if (!strcmp(point, mt->m_name)) {
			return(mt->m_entries->m_port);
		}
	}
	return(-1);
}
@


1.8
log
@Share port code in port.c
@
text
@d16 11
@


1.7
log
@Get rid of some debug noise
@
text
@a15 16
 * For mapping well-known-addresses into their port names
 */
static struct map {
	char *m_name;
	port_name m_addr;
} names[] = {
	{"NAMER", PORT_NAMER},
	{"TIMER", PORT_TIMER},
	{"ENV", PORT_ENV},
	{"CONS", PORT_CONS},
	{"KBD", PORT_KBD},
	{"SWAP", PORT_SWAP},
	{(char *)0, 0}
};

/*
d381 1
a381 1
	char *r, buf[80], *point, *path;
d383 1
a383 1
	port_name pn;
d408 2
a409 43
		 * See if we want to walk down into the port
		 * before mounting.
		 */
		path = strchr(buf, ':');
		if (path) {
			*path++ = '\0';
		}

		/*
		 * Numeric are used as-is
		 */
		if (isdigit(buf[0])) {
			pn = atoi(buf);

		/*
		 * Upper are well-known only
		 */
		} else if (isupper(buf[0])) {
			int x;

			for (x = 0; names[x].m_name; ++x) {
				if (!strcmp(names[x].m_name, buf)) {
					break;
				}
			}
			if (names[x].m_name == 0) {
				printf("mount: unknown port %s\n", buf);
				continue;
			}
			pn = names[x].m_addr;
		} else {
			/*
			 * Look up via namer for others
			 */
			pn = namer_find(buf);
			if (pn < 0) {
				printf("mount: can't find: %s\n", buf);
				continue;
			}
		}

		/*
		 * Connect to named port
d411 1
a411 1
		p = msg_connect(pn, ACC_READ);
a413 27
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
					perror(path);
					msg_disconnect(p);
					continue;
				}
				path = q;
			} while (path);
@


1.6
log
@Fix up umount() functionality, add a mount_init() to read a
standard file format for setting up mounts.  Add mount_port()
so we have something to use as the second argument to umount().
@
text
@a463 1
		printf("Name %s -> name %d\n", buf, pn);
a502 1
		printf(" ...mounted to %s port %d\n", point, p);
@


1.5
log
@Forgot to scale by size of mnttab
@
text
@d10 1
d16 16
a44 2
		int y;

d48 4
a51 6
		y = strcmp(point, __mnttab[x].m_name);

		/*
		 * On exact match, end loop
		 */
		if (!y) {
d101 2
d147 1
a147 1
umount(char *point, int fd)
a151 1
	port_t port;
d171 1
a171 1
	 * If "fd" given, look for particular slot
d173 1
a173 1
	if (fd >= 0) {
a175 1
		port = __fd_port(fd);
d194 5
d200 5
d391 2
a392 2
 * init_mount()
 *	Read mntrc, add any mount entries described within
d394 1
a394 2
void
init_mount(char *mntrc)
d397 3
a399 4
	char *p, buf[80];
	char *sympath, *point;
	port_name name;
	port_t port;
d401 2
a402 2
	if ((fp = fopen(mntrc, "r")) == 0) {
		return;
d404 1
a404 1
	while (fgets(buf, sizeof(buf), fp)) {
d406 1
a406 1
		 * Ignore comment lines
d414 1
a414 2
		 * Carve out first field--path to name in namer
		 * database.
d416 3
a418 3
		sympath = buf;
		p = strchr(buf, ':');
		if (p == 0) {
d421 16
d439 1
a439 1
		 * Second field is where to mount
d441 2
a442 2
		*p++ = '\0';
		point = p;
d444 22
d467 1
a467 1
		 * Look up port #
d469 3
a471 4
		name = namer_find(sympath);
		if (name < 0) {
			printf("Unknown resource: %s\n", sympath);
			continue;
d475 1
a475 1
		 * Connect to server
d477 22
a498 3
		port = msg_connect(name, ACC_READ);
		if (port < 0) {
			printf("Can't connect to: %s\n", sympath);
d502 1
a502 1
		 * Mount it
d504 22
a525 1
		mountport(point, port);
d527 1
@


1.4
log
@Add a routine to read an ASCII mount description and do
the described mounts.
@
text
@d194 2
a195 1
	bcopy(mt+1, mt, __nmnttab - (mt-__mnttab));
@


1.3
log
@Add interface to save/restore mount table on exec()
@
text
@d8 2
d364 65
@


1.2
log
@Fix mount table handling, forget about string ordering, and fix
some mistakes made during conversion of file descriptors to
ports.
@
text
@d195 168
@


1.1
log
@Initial revision
@
text
@d16 1
a16 1
mountport(char *point, port_t fd)
d18 1
a18 1
	int mntidx, x;
a24 1
	mntidx = -1;
a38 8

		/*
		 * Otherwise record where we would need to insert
		 * this entry.
		 */
		if ((y < 0) && (mntidx == -1)) {
			mntidx = y;
		}
a45 1
		close(fd);
d48 1
a48 1
	me->m_fd = fd;
a59 1
			close(fd);
d65 1
a65 2
		 * Open slot at required index if needed.  Leave "mt"
		 * pointing at it.
d67 1
a67 7
		if (mntidx != -1) {
			mt = __mnttab+mntidx;
			bcopy(mt, mt+1,
				(__nmnttab-mntidx)*sizeof(struct mnttab));
		} else {
			mt = __mnttab+__nmnttab;
		}
d74 10
d89 1
a89 1
	 * Our entry to the slot
d94 1
a94 1
	return(fd);
d115 5
a119 1
	return(mountport(point, __fd_port(fd)));
d130 1
a130 1
umount(char *point, port_t fd)
d135 1
d160 1
d166 1
a166 1
			if (me->m_fd == fd) {
d187 1
a187 1
		msg_disconnect(me->m_fd);
@
