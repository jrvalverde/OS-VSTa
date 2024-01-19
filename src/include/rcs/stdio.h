head	1.20;
access;
symbols
	V1_3_1:1.13
	V1_3:1.13
	V1_2:1.10
	V1_1:1.10
	V1_0:1.6;
locks; strict;
comment	@ * @;


1.20
date	94.12.23.04.15.12;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	94.12.21.05.38.01;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	94.10.13.14.18.19;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.09.26.17.09.58;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.09.23.20.38.37;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.08.27.00.14.33;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.05.30.21.30.14;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.04.11.00.34.51;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.02.02.19.57.20;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.02.01.23.24.08;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.10.17.19.26.35;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.10.09.03.22.01;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.09.18.18.16.57;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.08.29.22.55.16;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.16.19.12.18;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.11.19.18.00;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.10.18.44.25;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.08.23.04.51;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.26.18.45.46;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.48.53;	author vandys;	state Exp;
branches;
next	;


desc
@Standard I/O definitions
@


1.20
log
@Botched # args for putc
@
text
@#ifndef _STDIO_H
#define _STDIO_H
/*
 * stdio.h
 *	My poor little stdio emulation
 */
#include <sys/types.h>
#include <stdarg.h>

/*
 * A open, buffered file
 */
typedef struct __file {
	ushort f_flags;			/* See below */
	ushort f_fd;			/* File descriptor we use */
	char *f_buf, *f_pos;		/* Buffer */
	int f_bufsz, f_cnt;		/*  ...size, bytes queued */
	struct __file			/* List of all open files */
		*f_next, *f_prev;
} FILE;

/*
 * Bits in f_flags
 */
#define _F_WRITE (1)		/* Can write */
#define _F_READ (2)		/*  ...read */
#define _F_DIRTY (4)		/* Buffer holds new data */
#define _F_EOF (8)		/* Sticky EOF */
#define _F_ERR (16)		/*  ...error */
#define _F_UNBUF (32)		/* Flush after each put */
#define _F_LINE (64)		/*  ...after each newline */
#define _F_SETUP (128)		/* Buffers, etc. set up now */
#define _F_UBUF (256)		/* User-provided buffer */

/*
 * These need to be seen before getc()/putc() are defined
 */
extern int fgetc(FILE *), fputc(int, FILE *);

#ifdef __GNUC__
/*
 * The following in-line functions replace the more classic UNIX
 * approach of gnarly ?: constructs.  I think that inline C in
 * a .h is disgusting, but a step forward from unreadably
 * complex C expressions.
 */

/*
 * getc()
 *	Get a char from a stream
 */
static inline int
getc(FILE *fp)
{
	if (((fp->f_flags & (_F_READ|_F_DIRTY)) != _F_READ) ||
			(fp->f_cnt == 0)) {
		return(fgetc(fp));
	}
	fp->f_cnt -= 1;
	fp->f_pos += 1;
	return(fp->f_pos[-1]);
}

/*
 * putc()
 *	Put a char to a stream
 */
static inline int
putc(char c, FILE *fp)
{
	if (((fp->f_flags & (_F_WRITE|_F_SETUP|_F_DIRTY)) !=
			(_F_WRITE|_F_SETUP)) ||
			(fp->f_cnt >= fp->f_bufsz) ||
			((fp->f_flags & _F_LINE) && (c == '\n'))) {
		return(fputc(c, fp));
	}
	*(fp->f_pos) = c;
	fp->f_pos += 1;
	fp->f_cnt += 1;
	fp->f_flags |= _F_DIRTY;
	return(0);
}
#else

/*
 * For non-GNU C, just live with the function call overhead for now
 */
#define getc(f) fgetc(f)
#define putc(c, f) fputc(c, f)

#endif

/*
 * Smoke and mirrors
 */
#define getchar() getc(stdin)
#define putchar(c) putc(c, stdout)

/*
 * Pre-allocated stdio structs
 */
extern FILE *__iob, *__get_iob();
#define stdin (__iob)
#define stdout (__iob+1)
#define stderr (__iob+2)

/*
 * stdio routines
 */
extern FILE *fopen(const char *fname, const char *mode),
	*freopen(const char *, const char *, FILE *),
	*fdopen(int, const char *);
extern int fclose(FILE *),
	fread(void *, int, int, FILE *),
	fwrite(const void *, int, int, FILE *),
	feof(FILE *), ferror(FILE *),
	fileno(FILE *), ungetc(int, FILE *);
extern off_t fseek(FILE *, off_t, int), ftell(FILE *);
extern char *gets(char *), *fgets(char *, int, FILE *);
extern int puts(const char *), fputs(const char *, FILE *);
extern void clearerr(FILE *), setbuf(FILE *, char *),
	setbuffer(FILE *, char *, uint);
extern void rewind(FILE *);
extern int fflush(FILE *);
extern int printf(const char *, ...),
	fprintf(FILE *, const char *, ...),
	sprintf(char *, const char *, ...);
extern int vprintf(const char *, va_list),
	vfprintf(FILE *, const char *, va_list),
	vsprintf(char *, const char *, va_list);
extern int scanf(const char *, ...),
	fscanf(FILE *, const char *, ...),
	sscanf(char *, const char *, ...);
extern int vscanf(const char *, va_list),
	vfscanf(FILE *, const char *, va_list),
	vsscanf(char *, const char *, va_list);
extern FILE *tmpfile(void);

/*
 * Miscellany
 */
#if !defined(TRUE) && !defined(FALSE)
#define TRUE (1)
#define FALSE (0)
#endif
#define EOF (-1)
#if !defined(MIN) && !defined(MAX)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
#define BUFSIZ (4096)
#ifndef NULL
#define NULL (0)
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#endif

#endif /* _STDIO_H */
@


1.19
log
@Allow for non-GNU C compilers lacking the "inline" keyword
@
text
@d89 1
a89 1
#define putc(f) fputc(f)
@


1.18
log
@Add some const declarations
@
text
@d36 6
a48 5
 * These need to be seen before getc()/putc() are defined
 */
extern int fgetc(FILE *), fputc(int, FILE *);

/*
d83 9
@


1.17
log
@Fix def of __iob
@
text
@d100 3
a102 3
extern FILE *fopen(char *fname, char *mode),
	*freopen(char *, char *, FILE *),
	*fdopen(int, char *);
d105 1
a105 1
	fwrite(void *, int, int, FILE *),
d110 1
a110 1
extern int puts(char *), fputs(char *, FILE *);
@


1.16
log
@Add procedural interfaces to all global C library data
@
text
@d92 4
a95 4
extern FILE (*__iob)[];
#define stdin (&__iob[0])
#define stdout (&__iob[1])
#define stderr (&__iob[2])
@


1.15
log
@Add tmpfile()
@
text
@d92 1
a92 1
extern FILE __iob[3];
@


1.14
log
@varargs stuff
@
text
@d127 1
@


1.13
log
@Add proto for fflush()
@
text
@d8 1
a114 10

#ifndef __PRINTF_INTERNAL
/*
 * These prototypes are guarded by an #ifdef so that our actual
 * implementation can add an extra parameter, so that it can take
 * the address of the first arg from the stack.
 *
 * We *should* use varargs/stdargs, but these interfaces are
 * unpleasant.
 */
d118 9
a126 4
extern int scanf(char *, ...),
	fscanf(FILE *, char *, ...),
	sscanf(char *, char *, ...);
#endif
@


1.12
log
@Add rewind()
@
text
@d113 1
@


1.11
log
@Add protos
@
text
@d112 2
@


1.10
log
@Move fgetc()/fputc() up so their def can be seen by the in-line code
@
text
@d124 3
@


1.9
log
@Use inlining for getc/putc
@
text
@d42 5
a105 1
	fgetc(FILE *), fputc(int, FILE *),
@


1.8
log
@Screen some non-standard fluff
@
text
@d35 43
d80 2
a81 4
#define getc(f) fgetc(f)
#define putc(c, f) fputc(c, f)
#define getchar() fgetc(stdin)
#define putchar(c) fputc(c, stdout)
a100 1
	getc(FILE *), putc(int, FILE *),
@


1.7
log
@Add *printf() prototypes
@
text
@d85 1
d88 1
d90 1
d93 1
@


1.6
log
@Add user-provided buffer flag, as well as prototypes for
new buffering options.
@
text
@d68 13
@


1.5
log
@Add prototype for ungetc()
@
text
@d32 1
d53 3
a55 1
extern FILE *fopen(char *fname, char *mode);
d66 2
a67 1
extern void clearerr(FILE *), setbuf(FILE *, char *);
@


1.4
log
@Use anonymous parameter dec'ls
@
text
@d59 1
a59 1
	fileno(FILE *);
@


1.3
log
@Add prototypes for puts/fputs
@
text
@d54 2
a55 2
	fread(void *buf, int size, int nelem, FILE *f),
	fwrite(void *buf, int size, int nelem, FILE *f),
@


1.2
log
@Add some more standard fluff to stdio.h.
@
text
@d62 1
@


1.1
log
@Initial revision
@
text
@d58 2
a59 1
	fgetc(FILE *), fputc(int, FILE *);
d62 1
d73 9
@
