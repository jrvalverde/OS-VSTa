head	1.7;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.4
	V1_1:1.4
	V1_0:1.4;
locks; strict;
comment	@ * @;


1.7
date	94.10.28.04.44.54;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.10.13.16.44.09;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.04.02.22.00.42;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.05.23.31.34;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.26.18.41.57;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.21.21.43;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.23.18.20.14;	author vandys;	state Exp;
branches;
next	;


desc
@Wrapper for converting execl() into a exec() system call
@


1.7
log
@Fiddle const declarations, very intuitive
@
text
@/*
 * exec.c
 *	Wrapper for kernel exec() function
 */
#include <sys/mman.h>
#include <sys/exec.h>
#include <sys/fs.h>	/* For ENOEXEC */
#include <fcntl.h>
#include <std.h>
#include <mach/aout.h>
#include <sys/param.h>
#include <fdl.h>
#include <mnttab.h>

/*
 * execv()
 *	Execute a file with some arguments
 */
execv(const char *file, char * const *argv)
{
	int fd, x;
	uint plen, fdl_len, mnt_len, cwd_len;
	ulong narg;
	struct aout a;
	struct mapfile mf;
	char *p, *args;

	/*
	 * Open the file we're going to run
	 */
	fd = open(file, O_READ);
	if (fd < 0) {
		return(-1);
	}

	/*
	 * Read the header, verify its magic number
	 */
	if ((read(fd, &a, sizeof(a)) != sizeof(a)) ||
			((a.a_info & 0xFFFF) != 0413)) {
		close(fd);
		__seterr(ENOEXEC);
		return(-1);
	}

	/*
	 * Fill in the mapfile description from the a.out header
	 */
	bzero(&mf, sizeof(mf));

	/* Text */
	mf.m_map[0].m_vaddr = (void *)0x1000;
	mf.m_map[0].m_off = 0;
	mf.m_map[0].m_len = btorp(a.a_text + sizeof(a));
	mf.m_map[0].m_flags = M_RO;

	/* Data */
	mf.m_map[1].m_vaddr = (void *)roundup(
		(ulong)(mf.m_map[0].m_vaddr) + ptob(mf.m_map[0].m_len),
		0x400000);
	mf.m_map[1].m_off = mf.m_map[0].m_len;
	mf.m_map[1].m_len = btorp(a.a_data);
	mf.m_map[1].m_flags = 0;

	/* BSS */
	mf.m_map[2].m_vaddr = (char *)(mf.m_map[1].m_vaddr) +
		a.a_data;
	mf.m_map[2].m_off = 0;
	mf.m_map[2].m_len = btorp(a.a_bss);
	mf.m_map[2].m_flags = M_ZFOD;

	/* Entry point */
	mf.m_entry = (void *)(a.a_entry);

	/*
	 * Assemble arguments into a counted array
	 */
	plen = sizeof(ulong);
	for (narg = 0; argv[narg]; ++narg) {
		plen += (strlen(argv[narg])+1);
	}

	/*
	 * Add in length for fdl state and mount table
	 */
	fdl_len = __fdl_size();
	plen += fdl_len;
	mnt_len = __mount_size();
	plen += mnt_len;
	cwd_len = __cwd_size();
	plen += cwd_len;

	/*
	 * Create a shared mmap() area
	 */
	args = p = mmap(0, plen, PROT_READ|PROT_WRITE,
		MAP_ANON|MAP_SHARED, 0, 0L);
	if (p == 0) {
		return(-1);
	}

	/*
	 * Pack our arguments into it
	 */
	*(ulong *)p = narg;
	p += sizeof(ulong);
	for (narg = 0; argv[narg]; ++narg) {
		uint plen2;

		plen2 = strlen(argv[narg])+1;
		bcopy(argv[narg], p, plen2);
		p += plen2;
	}

	/*
	 * Add in our fdl state
	 */
	__fdl_save(p, fdl_len);
	p += fdl_len;

	/*
	 * And our mount state
	 */
	__mount_save(p);
	p += mnt_len;

	/*
	 * And our CWD
	 */
	__cwd_save(p);
	p += cwd_len;

	/*
	 * Here we go!
	 */
	return(exec(__fd_port(fd), &mf, args));
}

/*
 * execl()
 *	Alternate interface to exec(), simpler to call
 *
 * Interestingly, the arguments are already in the right format
 * on the stack.  We just use a little smoke and we're there.
 */
execl(const char *file, const char *arg0, ...)
{
	return(execv(file, (char **)&arg0));
}
@


1.6
log
@Add const to declarations
@
text
@d19 1
a19 1
execv(const char *file, const char **argv)
d148 1
a148 1
	return(execv(file, &arg0));
@


1.5
log
@Add check for a.out magic number
@
text
@d19 1
a19 1
execv(char *file, char **argv)
d146 1
a146 1
execl(char *file, char *arg0, ...)
@


1.4
log
@Add passing of CWD through exec()
@
text
@d7 1
d37 1
a37 1
	 * Read the header
d39 2
a40 1
	if (read(fd, &a, sizeof(a)) != sizeof(a)) {
d42 1
@


1.3
log
@Add execv() interface, also add code to save/restore mount table.
@
text
@d21 1
a21 1
	uint plen, fdl_len, mnt_len;
d87 2
d123 6
@


1.2
log
@Tidy up code, add in stuff to allow our fdl state to be passed
@
text
@d12 1
d15 1
a15 1
 * execl()
d18 1
a18 1
execl(char *file, char *arg0, ...)
d21 1
a21 1
	uint plen, fdl_len;
d25 1
a25 1
	char *p, **pp, *args;
d76 2
a77 4
	narg = 0;
	for (pp = &arg0; *pp; ++pp) {
		narg += 1;
		plen += (strlen(*pp)+1);
d81 1
a81 1
	 * Add in length for fdl state
d85 2
d102 1
a102 1
	for (pp = &arg0; *pp; ++pp) {
d105 2
a106 2
		plen2 = strlen(*pp)+1;
		bcopy(*pp, p, plen2);
d117 6
d128 11
@


1.1
log
@Initial revision
@
text
@d11 1
d19 2
a20 2
	int fd;
	uint plen;
d24 1
a24 1
	char *p, **pp;
d31 1
a31 1
		return(-9);
d39 1
a39 1
		return(-2);
d82 1
a82 1
	 * Create a shared mmap() area, assemble our arguments
d84 7
a90 1
	p = mmap(0, plen, PROT_READ|PROT_WRITE,
d93 1
a93 1
		return(-3);
d95 4
d100 1
a100 1
	plen = sizeof(ulong);
d105 2
a106 2
		bcopy(*pp, p+plen, plen2);
		plen += plen2;
d110 6
d118 1
a118 1
	return(exec(__fd_port(fd), &mf, p));
d120 1
@
