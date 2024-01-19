head	1.27;
access;
symbols
	V1_3_1:1.17
	V1_3:1.17
	V1_2:1.12
	V1_1:1.12
	V1_0:1.10;
locks; strict;
comment	@ * @;


1.27
date	95.01.10.05.13.43;	author vandys;	state Exp;
branches;
next	1.26;

1.26
date	94.10.28.04.44.54;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	94.10.13.14.18.19;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	94.10.04.19.49.29;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	94.10.01.03.35.38;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	94.09.23.20.38.54;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	94.08.27.00.15.59;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	94.07.10.18.24.44;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	94.06.04.19.26.18;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	94.05.24.17.11.05;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.04.11.00.34.39;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.04.06.03.36.59;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.04.02.01.53.44;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.03.28.23.09.12;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.03.23.21.53.02;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.08.29.22.55.29;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.08.24.04.54.48;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.08.03.00.00.08;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.06.30.19.54.50;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.04.12.20.56.54;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.04.09.17.13.08;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.10.18.44.12;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.05.23.31.49;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.18.20.25.55;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.05.14.22.24;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.01.15.45.02;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.48.46;	author vandys;	state Exp;
branches;
next	;


desc
@POSIX definitions
@


1.27
log
@Make uid_t/gid_t system types
Kernel cares about uid_t, and toss in gid_t to satisfy
principile of least astonishment
@
text
@#ifndef _STD_H
#define _STD_H
/*
 * std.h
 *	Another standards-driven file, I think
 *
 * See comments for unistd.h
 */
#include <sys/types.h>
#include <string.h>

/*
 * Routine templates
 */
extern void *malloc(unsigned int), *realloc(void *, unsigned int);
extern void *calloc(unsigned int, unsigned int);
extern void free(void *);
extern char *strerror( /* ... */ );
extern int fork(void), tfork(voidfun);
extern long __cwd_size(void);
extern void __cwd_save(char *);
extern char *__cwd_restore(char *);
extern char *getcwd(char *, int);
extern int dup(int), dup2(int, int);
extern int execl(const char *, const char *, ...),
	execv(const char *, char * const *),
	execlp(const char *, const char *, ...),
	execvp(const char *, char * const *);
extern char *getenv(const char *);
extern int setenv(const char *, const char *);
extern pid_t getpid(void), gettid(void), getppid(void);
extern int atoi(const char *);
extern long atol(const char *);
extern void perror(const char *);
extern uint sleep(uint);
extern int chdir(const char *);
extern int rmdir(const char *);
extern void *bsearch(const void *key, const void *base, size_t nmemb,
		     size_t size, int (*compar)(const void *, const void *));
extern void qsort(void *base, int n, unsigned size,
		  int (*compar)(void *, void *));
extern long strtol(const char *s, char **ptr, int base);
extern unsigned long strtoul(const char *s, char **ptr, int base);
extern int getdtablesize(void);
extern int system(const char *);
extern uid_t getuid(void);
extern gid_t getgid(void);

/*
 * GNU C has managed to change this one the last three times I moved
 * forward compilers, so don't blink your eyes.
 */
extern void exit(int), _exit(int);

/*
 * exit() values
 */
#define EXIT_SUCCESS (0)
#define EXIT_FAILURE (1)

#endif /* _STD_H */
@


1.26
log
@Fiddle const declarations, very intuitive
@
text
@d46 2
@


1.25
log
@Add some const declarations
@
text
@d26 1
a26 1
	execv(const char *, const char **),
d28 1
a28 1
	execvp(const char *, const char **);
@


1.24
log
@Move getopt() stuff to its own header file
@
text
@d25 6
a30 4
extern int execl(char *, char *, ...), execv(char *, char **),
	execlp(char *, char *, ...), execvp(char *, char **);
extern char *getenv(char *);
extern int setenv(char *, char *);
d45 1
d52 6
@


1.23
log
@Fix getopt() shlib hackery
@
text
@a44 10
 * getopt() package.  We use __getopt_ptr() to get a pointer to the
 * "global" data variables.
 */
extern void *__getopt_ptr(int);
#define optarg (*(char **)__getopt_ptr(3))
#define optind (*(int *)__getopt_ptr(1))
#define opterr (*(int *)__getopt_ptr(0))
extern int getopt(int, char **, char *);

/*
@


1.22
log
@Add procedural interfaces to all global C library data
@
text
@d49 3
a51 3
#define optarg (*(char **)__getopt(ptr(3)))
#define optind (*(int *)__getopt(ptr(1)))
#define opterr (*(int *)__getopt(ptr(0)))
@


1.21
log
@Add getdtablesize()
@
text
@d45 2
a46 1
 * getopt() package
d48 4
a51 2
extern char *optarg;
extern int optind, opterr;
@


1.20
log
@Add rmdir(), add some const definitions
@
text
@d42 1
@


1.19
log
@Add atol()
@
text
@d35 1
@


1.18
log
@Add qsort proto
@
text
@d31 1
@


1.17
log
@Add (yet another) attempt at the type of exit()
@
text
@d36 2
@


1.16
log
@Fix incompatibility w. gcc 1.x
@
text
@d46 6
@


1.15
log
@Add strto[u]l()
@
text
@d18 1
a18 1
extern char *strerror(...);
@


1.14
log
@Add some more POSIX stuff
@
text
@d36 2
@


1.13
log
@Add getopt() stuff
@
text
@d34 2
@


1.12
log
@Add some more ANSI prototypes
@
text
@d18 1
a18 1
extern char *strerror(void);
d34 7
@


1.11
log
@Add some missing functions, courtesy Gavin Nicol
@
text
@d31 3
@


1.10
log
@Add calloc()
@
text
@d30 1
@


1.9
log
@GCC warning cleanup
@
text
@d16 1
@


1.8
log
@Add prototypes for more interfaces, fix voidfun dec'l
@
text
@d10 1
d17 1
a17 2
extern char *strdup(char *), *strchr(char *, char), *strrchr(char *, char),
	*index(char *, char), *rindex(char *, char), *strerror(void);
a18 2
extern int bcopy(void *, void *, unsigned int),
	bcmp(void *, void *, unsigned int);
@


1.7
log
@Add prototypes for dup, dup2
@
text
@d9 1
a10 2
typedef void (*__voidfun)();

d18 1
a18 1
extern int fork(void), tfork(__voidfun);
d26 5
@


1.6
log
@Add getcwd() prototype
@
text
@d26 1
@


1.5
log
@Add prototypes for CWD state save/restore
@
text
@d25 1
@


1.4
log
@Add bcopy/bcmp prototypes
@
text
@d22 3
@


1.3
log
@Add strerror()
@
text
@d20 2
@


1.2
log
@Add prototypes for fork() and tfork()
@
text
@d18 1
a18 1
	*index(char *, char), *rindex(char *, char);
@


1.1
log
@Initial revision
@
text
@d10 2
d19 1
@
