head	1.8;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.5
	V1_1:1.5
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.8
date	94.06.03.04.46.33;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.10.23.41.13;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.03.28.23.09.12;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.24.04.54.48;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.16.17.09.23;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.06.30.19.54.50;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.16.19.12.38;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.13.01.40.18;	author vandys;	state Exp;
branches;
next	;


desc
@String prototypes
@


1.8
log
@Add strcspn()
@
text
@#ifndef _STRING_H
#define _STRING_H
/*
 * string.h
 *	Definition for string handling functions
 */
#include <sys/types.h>

/*
 * Prototypes
 */
extern char *strcpy(char *, const char *), *strncpy(char *, const char *, int);
extern char *strcat(char *, const char *), *strncat(char *, const char *, int);
extern size_t strlen(const char *);
extern int strcmp(const char *, const char *),
	strncmp(const char *, const char *, int);
extern void *memcpy(void *, const void *, size_t);
extern char *strchr(const char *, int), *strrchr(const char *, int);
extern char *index(const char *, int), *rindex(const char *, int);
extern char *strdup(const char *);
extern int bcmp(const void *, const void *, unsigned int),
	memcmp(const void *, const void *, size_t);
extern void bcopy(const void *, void *, unsigned int);
extern void bzero(void *, size_t);
extern char *strtok(char *, const char *),
	*strpbrk(const char *, const char *),
	*strstr(const char *, const char *);
extern size_t strspn(const char *, const char *);
extern void *memmove(void *, const void *, size_t),
	*memchr(const void *, unsigned char, size_t),
	*memset(void *, int, size_t);
extern char *strsep(char **, const char *);
extern void swab(const char *, char *, size_t);
extern size_t strcspn(const char *, const char *);

#endif /* _STRING_H */
@


1.7
log
@Add swab()
@
text
@d34 1
@


1.6
log
@Add some more POSIX stuff
@
text
@d33 1
@


1.5
log
@Add some missing functions, courtesy Gavin Nicol
@
text
@d32 1
@


1.4
log
@Update strlen() def to POSIX
@
text
@d24 9
@


1.3
log
@GCC warning cleanup
@
text
@d14 2
a15 1
extern int strlen(const char *), strcmp(const char *, const char *),
@


1.2
log
@Add memcmp()
@
text
@d7 1
d12 11
a22 11
extern char *strcpy(char *, char *), *strncpy(char *, char *, int);
extern char *strcat(char *, char *), *strncat(char *, char *, int);
extern int strlen(char *), strcmp(char *, char *),
	strncmp(char *, char *, int);
extern void *memcpy(void *, void *, unsigned int);
extern char *strchr(char *, char), *strrchr(char *, char);
extern char *index(char *, char), *rindex(char *, char);
extern char *strdup(char *);
extern int bcmp(void *, void *, unsigned int),
	memcmp(void *, void *, unsigned int);

@


1.1
log
@Initial revision
@
text
@d19 2
a20 1
extern int bcmp(void *, void *, unsigned int);
@
