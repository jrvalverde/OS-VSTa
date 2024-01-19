head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.4
	V1_1:1.4
	V1_0:1.3;
locks; strict;
comment	@ * @;


1.4
date	93.11.16.02.44.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.06.30.19.53.38;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.04.23.22.40.42;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.45.12;	author vandys;	state Exp;
branches;
next	;


desc
@Lexical analyzer
@


1.4
log
@Source reorg
@
text
@#ifdef KDB
/*
 * A standard lexical analyzer
 */
#include "../dbg/expr.h"

char *expr_line, *expr_pos;

static char buf[80];
static int donum();
extern int yylval;

int isdigit(char), isxdigit(char);

/*
 * Convert hex and decimal strings to integers
 */
xtoi(char *p)
{
	unsigned int val = 0;
	char c;

	while (c = *p++) {
		if (isdigit(c)) {
			val = val*16 + (c - '0');
		} else if (isxdigit(c)) {
			if (c < 'a') {
				val = val*16 + (c - 'A') + 10;
			} else {
				val = val*16 + (c - 'a') + 10;
			}
		} else {
			break;
		}
	}
	return(val);
}
atoi(char *p)
{
	unsigned int val = 0;
	char c;

	while (c = *p++) {
		if (isdigit(c)) {
			val = val*10 + (c - '0');
		} else {
			break;
		}
	}
	return(val);
}

 /*
  * Dummy functions so we don't have to load all of ctype
  */
static
isspace(char c)
{
	return((c == ' ') || (c == '\t'));
}
static
isalpha(char c)
{
	return(((c >= 'a') && (c <= 'z')) ||
		((c >= 'A') && (c <= 'Z')));
}
isdigit(char c)
{
	return((c >= '0') && (c <= '9'));
}
isxdigit(char c)
{
	return isdigit(c) || ((c >= 'a') && (c <= 'f')) ||
		((c >= 'A') && (c <= 'F'));
}
static
isalnum(char c)
{
	return isalpha(c) || isdigit(c);
}

 /*
  * getchar() function for lexical analyzer.
  */
static inline
nextc()
{
	register int c;

	/*
	 * Pop up a level of indirection on EOF
	 */
	c = *expr_pos;
	if (c == '\0') {
		return(-1);
	}
	expr_pos += 1;
	return (c);
}

/*
 * Push back a character
 */
static void inline
unget_c(c)
	int c;
{
	if ((expr_pos <= expr_line) || (c == -1) || !c) {
		return;
	}
	expr_pos -= 1;
	*expr_pos = c;
}

/*
 * Skip leading white space in current input stream
 */
static void
skipwhite()
{
	register c;

	/*
	 * Skip leading blank space
	 */
	while ((c = nextc()) != -1) {
		if (!isspace(c)) {
			break;
		}
	}
	unget_c(c);
}

/*
 * Lexical analyzer for YACC
 */
yylex()
{
	register char *p = buf;
	register c, c1;

	/*
	 * Skip over white space
	 */
again:
	skipwhite();
	c = nextc();

	/*
	 * Return EOF
	 */
	if (c == -1) {
		return (c);
	}

	/*
	 * An "identifier"?
	 */
	if (isalpha(c)) {
		/*
		 * Assemble a "word" out of the input stream, symbol table it
		 */
		*p++ = c;
		for (;;) {
			c = nextc();
			if (!isalnum(c) && (c != '_')) {
				break;
			}
			*p++ = c;
		}
		unget_c(c);
		*p = '\0';
		yylval = symval(buf);
		return (ID);
	}

	/*
	 * For numbers, call our number routine.
	 */
	if (isdigit(c)) {
		return (donum(c));
	}

	/*
	 * For certain C operators, need to look at following char to
	 *	assemble relationals.  Otherwise, just return the char.
	 */
	yylval = c;
	switch (c) {
	case '<':
		if ((c1 = nextc()) == '=') {
			return (LE);
		}
		unget_c(c1);
		return (c);
	case '>':
		if ((c1 = nextc()) == '=') {
			return (GE);
		}
		unget_c(c1);
		return (c);
	case '~':
		if ((c1 = nextc()) == '=') {
			return (NE);
		}
		unget_c(c1);
		return (c);
	default:
		return (c);
	}
}

/*
 * donum()
 *	Handle parsing of a number
 */
static int
donum(startc)
	char startc;
{
	register char c, *p = buf;

	/*
	 * Hex numbers
	 */
	if (startc == '0') {
		c = nextc();
		if (c == 'x') {
			c = nextc();
			while (isxdigit(c)) {
				*p++ = c;
				c = nextc();
			}
			unget_c(c);
			*p = '\0';
			yylval = xtoi(buf);
			return (INT);
		}
		unget_c(c);
	}

	/*
	 * Otherwise assume decimal
	 */
	*p++ = startc;
	for (;;) {
		c = nextc();
		if (isdigit(c)) {
			*p++ = c;
			continue;
		}
		unget_c(c);
		break;
	}
	*p = '\0';
	yylval = atoi(buf);
	return (INT);
}
#endif /* KDB */
@


1.3
log
@GCC warning cleanup
@
text
@d5 1
a5 1
#include <dbg/expr.h>
@


1.2
log
@Implement KDB
@
text
@d13 2
@


1.1
log
@Initial revision
@
text
@d1 1
a1 1
#ifdef DEBUG
d257 1
a257 1
#endif
@
