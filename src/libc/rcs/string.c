head	1.13;
access;
symbols
	V1_3_1:1.11
	V1_3:1.11
	V1_2:1.10
	V1_1:1.10
	V1_0:1.8;
locks; strict;
comment	@ * @;


1.13
date	94.06.04.19.25.56;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.06.03.04.45.55;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.02.28.22.01.51;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.08.24.04.54.48;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.08.16.17.09.33;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.07.07.22.15.51;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.06.30.19.54.36;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.05.06.23.25.52;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.04.21.22.36.28;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.16.19.08.33;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.13.01.31.26;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.19.15.35.01;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.01.59;	author vandys;	state Exp;
branches;
next	;


desc
@A bunch of string functions
@


1.13
log
@Fix edge condition for strchr/strrchr
@
text
@/*
 * string.c
 *	Miscellaneous string utilities
 */
#include <string.h>
#include <std.h>

/*
 * strcpy()
 *	Copy a string
 */
char *
strcpy(char *dest, const char *src)
{
	char *p = dest;

	while (*p++ = *src++)
		;
	return(dest);
}

/*
 * strncpy()
 *	Copy up to a limited length
 */
char *
strncpy(char *dest, const char *src, int len)
{
	char *p = dest, *lim;

	lim = p+len;
	while (p < lim) {
		if ((*p++ = *src++) == '\0') {
			break;
		}
	}
	return(dest);
}

/*
 * strlen()
 *	Length of string
 */
size_t
strlen(const char *p)
{
	size_t x = 0;

	while (*p++)
		++x;
	return(x);
}

/*
 * memcpy()
 *	The compiler can generate these; we use bcopy()
 */
void *
memcpy(void *dest, const void *src, size_t cnt)
{
	bcopy(src, dest, cnt);
	return(dest);
}

/*
 * strcmp()
 *	Compare two strings, return their relationship
 */
strcmp(const char *s1, const char *s2)
{
	while (*s1++ == *s2) {
		if (*s2++ == '\0') {
			return(0);
		}
	}
	return((int)s1[-1] - (int)s2[0]);
	return(1);
}

/*
 * strcat()
 *	Add one string to another
 */
char *
strcat(char *dest, const char *src)
{
	char *p;

	for (p = dest; *p; ++p)
		;
	while (*p++ = *src++)
		;
	return(dest);
}

/*
 * strncat()
 *	Concatenate, with limit
 */
char *
strncat(char *dest, const char *src, int len)
{
	char *p;
	const char *lim;

	lim = src+len;
	for (p = dest; *p; ++p)
		;
	while (src < lim) {
		if ((*p++ = *src++) == '\0') {
			return(dest);
		}
	}
	*p++ = '\0';
	return(dest);
}

/*
 * strchr()
 *	Return pointer to first occurence, or 0
 */
char *
strchr(const char *p, int c)
{
	int c2;

	do {
		c2 = *p++;
		if (c2 == c) {
			return((char *)(p-1));
		}
	} while (c2);
	return(0);
}

/*
 * strrchr()
 *	Like strchr(), but search backwards
 */
char *
strrchr(const char *p, int c)
{
	char *q = 0, c2;

	do {
		c2 = *p++;
		if (c == c2) {
			q = (char *)p;
		}
	} while (c2);
	return q ? (q-1) : 0;
}

/*
 * index/rindex()
 *	Alias for strchr/strrchr()
 */
char *
index(const char *p, int c)
{
	return(strchr(p, c));
}
char *
rindex(const char *p, int c)
{
	return(strrchr(p, c));
}

/*
 * strdup()
 *	Return malloc()'ed copy of string
 */
char *
strdup(const char *s)
{
	char *p;

	if (!s) {
		return(0);
	}
	if ((p = malloc(strlen(s)+1)) == 0) {
		return(0);
	}
	strcpy(p, s);
	return(p);
}

/*
 * strncmp()
 *	Compare up to a limited number of characters
 */
strncmp(const char *s1, const char *s2, int nbyte)
{
	while (nbyte && (*s1++ == *s2)) {
		if (*s2++ == '\0') {
			return(0);
		}
		nbyte -= 1;
	}
	if (nbyte == 0) {
		return(0);
	}
	return((int)s1[-1] - (int)s2[0]);
	return(1);
}

/*
 * bcmp()
 *	Compare, binary style
 */
bcmp(const void *s1, const void *s2, unsigned int n)
{
	const char *p = s1, *q = s2;

	while (n-- > 0) {
		if (*p++ != *q++) {
			return(1);
		}
	}
	return(0);
}

/*
 * memcmp()
 *	Yet Another Name, courtesy AT&T
 */
memcmp(const void *s1, const void *s2, size_t n)
{
	return(bcmp(s1, s2, n));
}

/*
 * strspn()
 *	Span the string s2 (skip characters that are in s2).
 */
size_t
strspn(const char *s1, const char *s2)
{
	const char *p = s1, *spanp;
	char c, sc;

	/* Skip any characters in s2, excluding the terminating \0. */
cont:
	c = *p++;
	for (spanp = s2; (sc = *spanp++) != 0; ) {
		if (sc == c)
			goto cont;
	}
	return (p - 1 - s1);
}

/*
 * strpbrk()
 *	Find the first occurrence in s1 of a character in s2 (excluding NUL)
 */
char *
strpbrk(const char *s1, const char *s2)
{
	const char *scanp;
	int c, sc;

	while ((c = *s1++) != 0) {
		for (scanp = s2; (sc = *scanp++) != 0; ) {
			if (sc == c) {
				return ((char *) (s1 - 1));
			}
		}
	}
	return(0);
}

/*
 * strstr()
 *	Find the first occurrence of find in s
 */
char *
strstr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return ((char *) 0);
			} while (sc != c);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *) s);
}

/*
 * strtok()
 *	Tokenize string
 */
char *
strtok(char *s, const char *delim)
{
	char *spanp;
	int c, sc;
	char *tok;
	static char *last;


	if (s == (char *) 0 && (s = last) == (char *) 0) {
		return(0);
	}

	/* Skip (span) leading delimiters (s += strspn(s, delim), sort of). */
cont:
	c = *s++;
	for (spanp = (char *) delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		last = 0;
		return(0);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *) delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = (char *) 0;
				else
					s[-1] = 0;
				last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/*NOTREACHED*/
}

/*
 * memmove()
 *	Apparently, a bcopy()
 */
void *
memmove(void *dest, const void *src, size_t length)
{
	bcopy(src, dest, length);
	return(dest);
}

/*
 * memchr()
 *	Like strchr(), but for any binary value
 */
void *
memchr(const void *s, unsigned char c, size_t n)
{
	if (n != 0) {
		const unsigned char *p = s;

		do {
			if (*p++ == c) {
				return ((void *) (p - 1));
			}
		} while (--n != 0);
	}
	return(0);
}

/*
 * memset()
 *	Set a binary range to a constant
 */
void *
memset(void *dst, int c, size_t n)
{
	if (n != 0) {
		char  *d = dst;

		do {
			*d++ = c;
		} while (--n != 0);
	}
	return (dst);
}

/*
 * strcspn()
 *	Find the length of initial part of s1 not including chars from s2
 */
size_t
strcspn(const char *s1, const char *s2)
{
	const char *s = s1;
	const char *c;

	while (*s1) {
		for (c = s2; *c; c++) {
			if (*s1 == *c) {
				break;
			}
		}
		if (*c) {
			break;
		}
		s1++;
	}

	return s1 - s;
}
@


1.12
log
@Add strcspn()
@
text
@d127 2
a128 1
	while (c2 = *p++) {
d132 1
a132 1
	}
d145 2
a146 1
	while (c2 = *p++) {
d150 1
a150 1
	}
@


1.11
log
@strncat() always '\0' terminates
@
text
@d391 25
@


1.10
log
@Add some missing functions, courtesy Gavin Nicol
@
text
@d111 1
a111 1
			break;
d114 1
@


1.9
log
@Update strlen() def to POSIX
@
text
@d228 162
@


1.8
log
@Semantics wrong for count on strncat
@
text
@d44 1
d47 1
a47 1
	int x = 0;
@


1.7
log
@GCC warning cleanup
@
text
@d99 2
a100 1
char *strncat(char *dest, const char *src, int len)
d102 2
a103 1
	char *p, *lim;
d105 1
a105 1
	lim = dest+len;
d108 1
a108 1
	while (p < lim) {
@


1.6
log
@Fix return value for strcpy(); was returning dest pointer value
after it had been bumped to end of string.
@
text
@d13 1
a13 1
strcpy(char *dest, char *src)
d27 1
a27 1
strncpy(char *dest, char *src, int len)
d44 1
a44 1
strlen(char *p)
d58 1
a58 1
memcpy(void *dest, void *src, unsigned int cnt)
d68 1
a68 1
strcmp(char *s1, char *s2)
d84 1
a84 1
strcat(char *dest, char *src)
d99 1
a99 1
char *strncat(char *dest, char *src, int len)
d119 1
a119 1
strchr(char *p, char c)
d121 1
a121 1
	char c2;
d125 1
a125 1
			return(p-1);
d136 1
a136 1
strrchr(char *p, char c)
d142 1
a142 1
			q = p;
d153 1
a153 1
index(char *p, char c)
d158 1
a158 1
rindex(char *p, char c)
d168 1
a168 1
strdup(char *s)
d186 1
a186 1
strncmp(char *s1, char *s2, int nbyte)
d205 1
a205 1
bcmp(void *s1, void *s2, unsigned int n)
d207 1
a207 1
	char *p = s1, *q = s2;
d221 1
a221 1
memcmp(void *s1, void *s2, unsigned int n)
@


1.5
log
@Backed up the wrong pointer
@
text
@d15 3
a17 1
	while (*dest++ = *src++)
d29 1
a29 1
	char *lim;
d31 3
a33 3
	lim = dest+len;
	while (dest < lim) {
		if ((*dest++ = *src++) == '\0') {
@


1.4
log
@Add memcmp()
@
text
@d195 1
a195 1
	return((int)s1[0] - (int)s2[-1]);
@


1.3
log
@Add strncpy()/strncat()
@
text
@d214 9
@


1.2
log
@Add bcmp()
@
text
@d5 1
d21 18
d90 19
@


1.1
log
@Initial revision
@
text
@d160 16
@
