head	1.9;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.7
	V1_1:1.7
	V1_0:1.7;
locks; strict;
comment	@ * @;


1.9
date	94.09.26.17.13.37;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.09.23.20.37.41;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.07.09.18.37.00;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.27.00.31.32;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.22.23.20.53;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.05.23.31.18;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.26.18.42.57;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.21.22.27;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.01.12;	author vandys;	state Exp;
branches;
next	;


desc
@C code called during crt0 startup.  Responsible for generating
argc/argv and such.
@


1.9
log
@Fix definitions of ctab and iob
@
text
@/*
 * __start()
 *	Do the VSTa-specific stuff to set up argc/argv
 */
#include <sys/types.h>
#include <std.h>
#include <fdl.h>
#include <mnttab.h>
#include <ctype.h>
#include <stdio.h>

/*
 * We initialize these here and offer them to C library users
 */
const unsigned char *__ctab;
FILE *__iob;

/*
 * __start()
 *	Do some massaging of arguments during C startup
 */
void
#ifdef SRV
__start2
#else
__start
#endif
(int *argcp, char ***argvp, char *a)
{
	char *p, **argv = 0;
	int x, argc = 0, len;
	char *basemem;

	/*
	 * Boot programs get this
	 */
	if (a == 0) {
#ifdef SRV
		extern int __bootargc;
		extern char *__bootargv;
		extern char __bootarg[];

		if (__bootargc) {
			*argcp = __bootargc;
			*argvp = &__bootargv;
			set_cmd(__bootargv);
			return;
		}
#endif
noargs:
		*argcp = 0;
		*argvp = 0;
		return;
	}
	basemem = a;

	/*
	 * Get count of arguments
	 */
	argc = *(ulong *)a;
	a += sizeof(ulong);
	argv = malloc((argc+1) * sizeof(char *));
	if (argv == 0) {
		goto noargs;
	}

	/*
	 * Walk the argument area.  Copy each string, as we wish
	 * to tear down the shared memory once we're finished.
	 */
	for (x = 0; x < argc; ++x) {
		len = strlen(a)+1;
		argv[x] = malloc(len);
		if (!argv[x]) {
			goto noargs;
		}
		bcopy(a, argv[x], len);
		a += len;
	}
	argv[x] = 0;	/* Traditional */

	/*
	 * Stuff our values back to our caller
	 */
	*argcp = argc;
	*argvp = argv;

	/*
	 * Set command
	 */
	if (p = strrchr(argv[0], '/')) {
		(void)set_cmd(p+1);
	} else {
		(void)set_cmd(argv[0]);
	}

	/*
	 * Restore our fdl state
	 */
	a = __fdl_restore(a);

	/*
	 * Restore mount table
	 */
	a = __mount_restore(a);

	/*
	 * Restore current working directory
	 */
	a = __cwd_restore(a);

	/*
	 * Unmap the argument memory
	 */
	(void)munmap(basemem, a-basemem);

	/*
	 * Wire in <ctype.h> ctab pointer and <stdio.h> iob[]
	 */
	__ctab = __get_ctab();
	__iob  = __get_iob();
}
@


1.8
log
@Create procedural interfaces to all global C library data
@
text
@d15 2
a16 2
unsigned char *__ctab;
FILE (*__iob)[];
@


1.7
log
@For boot server startup, use arguments provided in special
reserved area if they're present.
@
text
@d9 2
d13 6
d116 6
@


1.6
log
@Set command name if available
@
text
@d15 6
a20 1
__start(int *argcp, char ***argvp, char *a)
d30 12
@


1.5
log
@Unmap the argument memory once it's been used to restore
context.
@
text
@d64 9
@


1.4
log
@Add passing of CWD through exec()
@
text
@d19 1
d30 1
d76 6
a81 1
	__cwd_restore(a);
@


1.3
log
@Add restore of mount table
@
text
@d69 6
a74 1
	__mount_restore(a);
@


1.2
log
@Add code to restore state from exec()--fdl and command line
@
text
@d8 1
d64 6
a69 1
	__fdl_restore(a);
@


1.1
log
@Initial revision
@
text
@d5 3
a7 2
#include <sys/vm.h>
#include <lib/alloc.h>
d14 1
a14 1
__start(int *argcp, char ***argvp)
a15 1
#ifdef LATER
d17 1
a17 1
	int argc = 0;
d20 1
a20 3
	 * Walk the argument area.  Each argument is terminated with
	 * a null byte; two nulls in a row marks the end of all
	 * arguments.
d22 26
a47 6
	for (p = VADDR_ARGS; *p; ++p) {
		argc += 1;
		argv = realloc(argv, sizeof(char *)*argc);
		argv[argc-1] = p;
		while (*p) {
			++p;
d49 2
d52 1
d59 5
a63 3
#else
	return;
#endif
@
