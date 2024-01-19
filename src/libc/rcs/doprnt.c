head	1.6;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.6
date	94.08.27.00.13.57;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.08.03.22.04.06;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.10.03.00.25.56;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.14.01.11.23;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.05.16.04.13;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.59.02;	author vandys;	state Exp;
branches;
next	;


desc
@Underlying engine for all printf()-style interfaces
@


1.6
log
@Initialize precision to 0
@
text
@/*
 * doprnt.c
 *	Low-level machinery used by all flavors of printf()
 */
#define NUMBUF (32)

/*
 * num()
 *	Convert number to string
 *
 * Returns length of resulting string
 */
static
num(char *buf, unsigned int x, unsigned int base, int is_unsigned)
{
	char *p = buf+NUMBUF;
	unsigned int c, len = 1, neg = 0;

	/*
	 * Only decimal is signed
	 */
	if ((base == 10) && !is_unsigned) {
		if ((int)x < 0) {
			neg = 1;
			x = -(int)x;
		}
	}
	*--p = '\0';
	do {
		c = (x % base);
		if (c < 10) {
			*--p = '0'+c;
		} else {
			*--p = 'a'+(c-10);
		}
		len += 1;
		x /= base;
	} while (x != 0);

	/*
	 * Add leading '-' if negative
	 */
	if (neg) {
		*--p = '-';
		len += 1;
	}

	/*
	 * Move numeric image to front of buffer
	 */
	bcopy(p, buf, len);
	return(len-1);
}

/*
 * baseof()
 *	Given character, return base value
 */
static
baseof(char c)
{
	switch (c) {
	case 'u':
	case 'd':
	case 'U':
	case 'D':
		return(10);
	case 'x':
	case 'X':
		return(16);
	case 'o':
	case 'O':
		return(8);
	default:
		return(10);
	}
}

/*
 * __doprnt()
 *	Do printf()-style printing
 */
void
__doprnt(char *buf, char *fmt, int *args)
{
	char *p = fmt, c;
	char numbuf[NUMBUF];
	int adj, width, zero, longfmt, x, is_unsigned,
		dotf, precision;

	while (c = *p++) {
		/*
		 * Non-format; use character
		 */
		if (c != '%') {
			*buf++ = c;
			continue;
		}
		c = *p++;

		/*
		 * Leading '-'; toggle default adjustment
		 */
	 	if (c == '-') {
			adj = 1;
			c = *p++;
		} else {
			adj = 0;
		}

		/*
		 * Leading 0; zero-fill
		 */
	 	if (c == '0') {
			zero = 1;
			c = *p++;
		} else {
			zero = 0;
		}

		/*
		 * Numeric; field width
		 */
		if (isdigit(c)) {
			width = atoi(p-1);
			while (isdigit(*p))
				++p;
			c = *p++;
		} else {
			width = 0;
		}

		/*
		 * '.'; precision for numeric, max length * for strings.
		 */
		if (c == '.') {
			dotf = 1;
			c = *p++;
			if (isdigit(c)) {
				precision = atoi(p-1);
				while (isdigit(*p))
					++p;
				c = *p++;
			} else if (c == '*') {
				precision = *args++;
				c = *p++;
			}
		} else {
			precision = dotf = 0;
		}

		/*
		 * 'l': "long" format.  XXX Use this when sizeof(int)
		 * stops being sizeof(long).
		 */
		if (c == 'l') {
			longfmt = 1;
			c = *p++;
		} else {
			longfmt = 0;
		}

		/*
		 * 'u': unsigned
		 */
		if (c == 'u') {
			is_unsigned = 1;
		} else {
			is_unsigned = 0;
		}

		/*
		 * Format
		 */
		switch (c) {
		case 'X':
		case 'O':
		case 'D':
		case 'U':
			longfmt = 1;
			/* VVV fall into VVV */

		case 'x':
		case 'o':
		case 'd':
		case 'u':
			x = num(numbuf, *args++, baseof(c), is_unsigned);
			if (!adj) {
				for ( ; x < width; ++x) {
					*buf++ = zero ? '0' : ' ';
				}
			}
			strcpy(buf, numbuf);
			buf += strlen(buf);
			if (adj) {
				for ( ; x < width; ++x) {
					*buf++ = ' ';
				}
			}
			break;
			num(numbuf, *args++, 16, is_unsigned);
			strcpy(buf, numbuf);
			buf += strlen(buf);
			break;

		case 's':
			if (args[0] == 0) {
				args[0] = (int)"(null)";
			}
			x = strlen((char *)(args[0]));
			if (precision && (x > precision)) {
				x = precision;
			}
			if (!adj) {
				while (width-- > x) {
					*buf++ = ' ';
				}
			}
			bcopy(*args++, buf, x);
			buf[x] = '\0';
			buf += x;
			if (adj) {
				while (width-- > x) {
					*buf++ = ' ';
				}
			}
			break;

		case 'c':
			x = *args++;
			*buf++ = x;
			break;

		default:
			*buf++ = c;
			break;
		}
	}
	*buf = '\0';
}
@


1.5
log
@Add ./* formats for strings
@
text
@a128 1
			precision = 0;
d149 1
a149 1
			dotf = 0;
@


1.4
log
@Make 'u' as valid as 'd', not a modifier
@
text
@d88 2
a89 1
	int adj, width, zero, longfmt, x, is_unsigned;
d129 1
d135 19
d212 3
d216 1
a216 1
				for ( ; x < width; ++x) {
d220 3
a222 2
			strcpy(buf, (char *)(*args++));
			buf += strlen(buf);
d224 1
a224 1
				for ( ; x < width; ++x) {
@


1.3
log
@Display null strings under %s as "(null)"
@
text
@d63 1
d65 1
a147 1
			c = *p++;
d159 1
d166 1
@


1.2
log
@Add signed/unsigned support for %d(%ud)
@
text
@d184 3
@


1.1
log
@Initial revision
@
text
@d14 1
a14 1
num(char *buf, unsigned int x, unsigned int base)
d17 1
a17 1
	unsigned int c, len = 1;
d19 9
d39 12
d86 1
a86 1
	int adj, width, zero, longfmt, x;
d132 1
a132 1
		 * stop being sizeof(long).
d142 10
a154 1
		case 'D':
d157 1
a160 1
		case 'd':
d163 2
a164 1
			x = num(numbuf, *args++, baseof(c));
d178 1
a178 1
			num(numbuf, *args++, 16);
@
