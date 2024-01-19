head	1.18;
access;
symbols
	V1_3_1:1.13
	V1_3:1.13
	V1_2:1.10
	V1_1:1.10
	V1_0:1.9;
locks; strict;
comment	@ * @;


1.18
date	94.10.23.18.10.47;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.10.13.14.17.59;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.09.23.20.37.41;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	94.08.27.00.14.18;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	94.07.10.19.27.05;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.04.11.00.34.24;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.02.02.19.57.20;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.02.01.23.22.45;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.10.09.03.21.45;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.08.04.22.29.44;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.03.20.00.21.49;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.03.16.19.08.55;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.13.01.31.58;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.11.19.13.48;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.10.18.43.35;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.08.23.03.42;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.26.18.44.24;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.01.49;	author vandys;	state Exp;
branches;
next	;


desc
@Most of the buffered <stdio.h> routines
@


1.18
log
@Fix seeking, get rid of useless assignment
@
text
@/*
 * stdio.c
 *	Stuff in support of standard I/O
 */
#include <stdio.h>
#include <unistd.h>
#include <std.h>
#include <fcntl.h>
#include <alloc.h>

/*
 * Memory for the three predefined files.  If stdin/stdout are hooked
 * to terminals the fill/flush vectors will be set to routines which
 * do TTY-style processing.
 */
FILE __iob2[3] = {
	{_F_READ, 0, 0, 0, 0, 0, 0, 0},
	{_F_WRITE|_F_LINE, 1, 0, 0, 0, 0, 0, 0},
	{_F_WRITE|_F_UNBUF, 2, 0, 0, 0, 0, 0, 0}
};

/*
 * Return pointer to iob array
 */
FILE *
__get_iob(void)
{
	return(__iob2);
}

/*
 * Used to enumerate all open files
 */
static FILE *allfiles = 0;

/*
 * add_list()
 *	Add a FILE to a list
 */
static void
add_list(FILE **hd, FILE *fp)
{
	FILE *f = *hd;

	/*
	 * First entry
	 */
	if (f == 0) {
		*hd = fp;
		fp->f_next = fp->f_prev = fp;
		return;
	}

	/*
	 * Insert at tail
	 */
	fp->f_next = f;
	fp->f_prev = f->f_prev;
	f->f_prev = fp;
	fp->f_prev->f_next = fp;
}

/*
 * del_list()
 *	Delete FILE from list
 */
static void
del_list(FILE **hd, FILE *fp)
{
	/*
	 * Ignore those which were never active
	 */
	if (fp->f_next == 0) {
		return;
	}

	/*
	 * Last entry
	 */
	if (fp->f_next == fp) {
		*hd = 0;
		return;
	}

	/*
	 * Delete from list
	 */
	fp->f_next->f_prev = fp->f_prev;
	fp->f_prev->f_next = fp->f_next;
	*hd = fp->f_next;
}

/*
 * fillbuf()
 *	Fill buffer from block-type device
 *
 * Returns 0 on success, 1 on error.
 */
static
fillbuf(FILE *fp)
{
	int x;

	/*
	 * Always start with fresh buffer
	 */
	fp->f_pos = fp->f_buf;
	fp->f_cnt = 0;

	/*
	 * Hard EOF/ERR
	 */
 	if (fp->f_flags & (_F_EOF|_F_ERR)) {
		return(1);
	}

	/*
	 * Read next buffer-full
	 */
	x = read(fp->f_fd, fp->f_buf, fp->f_bufsz);

	/*
	 * On error, leave flag (hard errors) and return
	 */
	if (x <= 0) {
		fp->f_flags |= ((x == 0) ? _F_EOF : _F_ERR);
		return(1);
	}

	/*
	 * Update count in buffer, return success
	 */
	fp->f_cnt = x;
	return(0);
}

/*
 * flushbuf()
 *	Flush block-type buffer
 *
 * Returns 0 on success, 1 on error.
 */
static
flushbuf(FILE *fp)
{
	int x, cnt;

#ifdef XXX
	/*
	 * Hard EOF/ERR
	 * Desirable on output?  Seems not to me....
	 */
 	if (fp->f_flags & (_F_EOF|_F_ERR)) {
		return(1);
	}
#endif

	/*
	 * No data movement--always successful
	 */
	if (fp->f_cnt == 0) {
		return(0);
	}

	/*
	 * Write next buffer-full
	 */
	cnt = fp->f_cnt;
	x = write(fp->f_fd, fp->f_buf, cnt);

	/*
	 * Always leave fresh buffer
	 */
	fp->f_pos = fp->f_buf;
	fp->f_cnt = 0;
	fp->f_flags &= ~_F_DIRTY;

	/*
	 * On error, leave flag (hard errors) and return
	 */
	if (x != cnt) {
		fp->f_flags |= ((x < 0) ? _F_ERR : _F_EOF);
		return(1);
	}

	/*
	 * Return success
	 */
	return(0);
}

/*
 * setup_fp()
 *	Get buffers
 */
static
setup_fp(FILE *fp)
{
	/*
	 * This handles stdin/out/err; opens via fopen() already
	 * have their buffer.
	 */
	if (fp->f_buf == 0) {
		/*
		 * Set up buffer
		 */
		fp->f_buf = malloc(BUFSIZ);
		if (fp->f_buf == 0) {
			return(1);
		}
		fp->f_bufsz = BUFSIZ;
		fp->f_pos = fp->f_buf;
		fp->f_cnt = 0;

		/*
		 * Add to "all files" list
		 */
		if (fp->f_next == 0) {
			add_list(&allfiles, fp);
		}
	}

	/*
	 * Flag set up
	 */
	fp->f_flags |= _F_SETUP;
	return(0);
}

/*
 * set_read()
 *	Do all the cruft needed for a read
 *
 * Returns zero on success, 1 on error.
 * This represents a lot more sanity checking than most implementations.
 * XXX does it hurt enough to throw out?
 */
static
set_read(FILE *fp)
{
	/*
	 * Allowed?
	 */
	if ((fp->f_flags & _F_READ) == 0) {
		return(1);
	}

	/*
	 * If switching from write to read, flush dirty stuff out
	 */
	if (fp->f_flags & _F_DIRTY) {
		flushbuf(fp);
		fp->f_flags &= ~_F_DIRTY;
	}

	/*
	 * Fill buffer if nothing in it
	 */
	if (fp->f_cnt == 0) {
		/*
		 * Do one-time setup
		 */
	 	if ((fp->f_flags & _F_SETUP) == 0) {
			if (setup_fp(fp)) {
				return(1);
			}
		}

		/*
		 * Call fill routine
		 */
		if (fillbuf(fp)) {
			return(1);
		}
	}
	return(0);
}

/*
 * set_write()
 *	Set FILE for writing
 *
 * Returns 1 on error, 0 on success.
 */
static
set_write(FILE *fp)
{
	/*
	 * Allowed?
	 */
	if ((fp->f_flags & _F_WRITE) == 0) {
		return(1);
	}

	/*
	 * Do one-time setup
	 */
	if ((fp->f_flags & _F_SETUP) == 0) {
		if (setup_fp(fp)) {
			return(1);
		}
	}

	/*
	 * Switching modes?
	 */
	if (!(fp->f_flags & _F_DIRTY)) {
		/*
		 * Have to get file position back to where last user-seen
		 * read finished.
		 */
		if (fp->f_cnt) {
			if (lseek(fp->f_fd,
				lseek(fp->f_fd, 0L, SEEK_CUR) - fp->f_cnt,
				 SEEK_SET) == -1) {
					return(1);
			}
		}

		/*
		 * Nothing there, ready for action
		 */
		fp->f_cnt = 0;
		fp->f_pos = fp->f_buf;
		fp->f_flags |= _F_DIRTY;
	}

	return(0);
}

/*
 * fgetc()
 *	Get a character from the FILE
 */
fgetc(FILE *fp)
{
	uchar c;

	/*
	 * Sanity for reading
	 */
 	if (set_read(fp)) {
		return(EOF);
	}

	/*
	 * Pull next character from buffer
	 */
	c = *(fp->f_pos);
	fp->f_pos += 1; fp->f_cnt -= 1;
	return(c);
}

/*
 * fputc()
 *	Put a character to a FILE
 */
fputc(int c, FILE *fp)
{
	/*
	 * Sanity for writing
	 */
	if (set_write(fp)) {
		return(EOF);
	}

	/*
	 * Flush when full
	 */
	if (fp->f_cnt >= fp->f_bufsz) {
		if (flushbuf(fp)) {
			return(EOF);
		}
	}

	/*
	 * Add to buffer
	 */
	*(fp->f_pos) = c;
	fp->f_pos += 1;
	fp->f_cnt += 1;
	fp->f_flags |= _F_DIRTY;

	/*
	 * If line-buffered and newline or unbuffered, flush
	 */
	if (fp->f_flags & (_F_LINE|_F_UNBUF)) {
		if (((fp->f_flags & _F_LINE) && (c == '\n')) ||
				(fp->f_flags & _F_UNBUF)) {
			if (flushbuf(fp)) {
				return(EOF);
			}
			return(0);
		}
	}

	/*
	 * Otherwise just leave in buffer
	 */
	return(0);
}

/*
 * fdopen()
 *	Open FILE on an existing file descriptor
 */
FILE *
fdopen(int fd, const char *mode)
{
	char c;
	const char *p;
	int m = 0, x;
	FILE *fp;

	/*
	 * Get FILE *
	 */
	if ((fp = malloc(sizeof(FILE))) == 0) {
		return(0);
	}

	/*
	 * Get data buffer
	 */
	if ((fp->f_buf = malloc(BUFSIZ)) == 0) {
		free(fp);
		return(0);
	}

	/*
	 * Interpret mode of file open
	 */
	for (p = mode; c = *p; ++p) {
		switch (c) {
		case 'r':
			m |= _F_READ;
			break;
		case 'w':
			m |= _F_WRITE;
			break;
		case 'b':
			m |= /* _F_BINARY */ 0 ;
			break;
		case '+':
			m |= _F_WRITE | _F_READ;
			break;
		default:
			free(fp->f_buf);
			free(fp);
			return(0);
		}
	}

	/*
	 * Set up rest of fp now that we know it's worth it
	 */
	fp->f_fd = fd;
	fp->f_flags = m;
	fp->f_pos = fp->f_buf;
	fp->f_bufsz = BUFSIZ;
	fp->f_cnt = 0;
	fp->f_next = 0;

	/*
	 * Insert in "all files" list
	 */
	add_list(&allfiles, fp);

	return(fp);
}

/*
 * freopen()
 *	Open a buffered file on given FILE
 */
FILE *
freopen(const char *name, const char *mode, FILE *fp)
{
	const char *p;
	char c;
	int m = 0, o = 0, x;

	/*
	 * Flush old data, if any
	 */
	if (fp->f_flags & _F_DIRTY) {
		fflush(fp);
	}
	if (fp->f_buf && !(fp->f_flags & _F_UBUF)) {
		free(fp->f_buf);
	}

	/*
	 * Get data buffer
	 */
	if ((fp->f_buf = malloc(BUFSIZ)) == 0) {
		return(0);
	}

	/*
	 * Interpret mode of file open
	 */
	for (p = mode; c = *p; ++p) {
		switch (c) {
		case 'r':
			m |= _F_READ;
			o |= O_READ;
			break;
		case 'w':
			m |= _F_WRITE;
			o |= O_TRUNC|O_WRITE;
			break;
		case 'b':
			m |= /* _F_BINARY */ 0 ;
			o |= O_BINARY;
			break;
		case '+':
			m |= (_F_WRITE | _F_READ);
			o |= (O_WRITE | O_READ);
			break;
		default:
			return(0);
		}
	}

	/*
	 * Open file
	 */
	x = open(name, o);
	if (x < 0) {
		return(0);
	}
	fp->f_fd = x;

	/*
	 * Set up rest of fp now that we know it's worth it
	 */
	fp->f_flags = m;
	fp->f_pos = fp->f_buf;
	fp->f_bufsz = BUFSIZ;
	fp->f_cnt = 0;
	fp->f_next = 0;

	/*
	 * Insert in "all files" list
	 */
	add_list(&allfiles, fp);

	return(fp);
}

/*
 * fopen()
 *	Open buffered file
 */
FILE *
fopen(const char *name, const char *mode)
{
	FILE *fp;

	/*
	 * Get FILE *
	 */
	if ((fp = malloc(sizeof(FILE))) == 0) {
		return(0);
	}

	/*
	 * freopen() looks at these
	 */
	fp->f_flags = 0;
	fp->f_buf = 0;

	/*
	 * Try to open file on it
	 */
	if (freopen(name, mode, fp) == 0) {
		free(fp);
		return(0);
	}
	return(fp);
}

/*
 * fclose()
 *	Close a buffered file
 */
fclose(FILE *fp)
{
	int err = 0;

	/*
	 * Flush the buffer if necessary
	 */
	if (fp->f_flags & _F_DIRTY) {
		flushbuf(fp);
	}

	/*
	 * Remove from "all files" list
	 */
	del_list(&allfiles, fp);

	/*
	 * Close fd, free buffer space
	 */
	err |= close(fp->f_fd);
	if (fp->f_buf && !(fp->f_flags & _F_UBUF)) {
		free(fp->f_buf);
	}
	if ((fp < &__iob2[0]) || (fp >= &__iob2[3])) {
		free(fp);
	}

	return(err);
}

/*
 * fflush()
 *	Flush out buffers in FILE
 */
int
fflush(FILE *fp)
{
	/*
	 * No data--already flushed
	 */
	if ((fp->f_flags & _F_DIRTY) == 0) {
		return(0);
	}

	/*
	 * Call flush
	 */
	if (flushbuf(fp)) {
		return(EOF);
	}
	return(0);
}

/*
 * __allclose()
 *	Close all open FILE's
 */
void
__allclose(void)
{
	int x;
	FILE *fp, *start;

	/*
	 * Walk list of all files, flush out dirty buffers
	 */
	fp = start = allfiles;
	if (!fp) {
		return;
	}
	do {
		if (fp->f_flags & _F_DIRTY) {
			fflush(fp);
		}
		close(fp->f_fd);
		fp = fp->f_next;
	} while (fp != start);
}

/*
 * gets()
 *	Get a string, discard terminating newline
 *
 * Both gets() and fgets() have extra brain damage to coexist with
 * MS-DOS editors.
 */
char *
gets(char *buf)
{
	int c;
	char *p = buf;

	while ((c = fgetc(stdin)) != EOF) {
		if (c == '\r') {
			continue;
		}
		if (c == '\n') {
			break;
		}
		*p++ = c;
	}
	if (c == EOF) {
		return(0);
	}
	*p = '\0';
	return(buf);
}

/*
 * fgets()
 *	Get a string, keep terminating newline
 */
char *
fgets(char *buf, int len, FILE *fp)
{
	int x = 0, c;
	char *p = buf;

	while ((x++ < (len - 1)) && ((c = fgetc(fp)) != EOF)) {
		if (c == '\r') {
			continue;
		}
		*p++ = c;
		if (c == '\n') {
			break;
		}
	}
	if (c == EOF) {
		return(0);
	}
	*p++ = '\0';
	return(buf);
}

/*
 * ferror()
 *	Tell if there's an error on the buffered file
 */
ferror(FILE *fp)
{
 	return (fp->f_flags & _F_ERR);
}

/*
 * feof()
 *	Tell if at end of media
 */
feof(FILE *fp)
{
 	return (fp->f_flags & _F_EOF);
}

/*
 * fileno()
 *	Return file descriptor value
 */
fileno(FILE *fp)
{
	return (fp->f_fd);
}

/*
 * clearerr()
 *	Clear error state on file
 */
void
clearerr(FILE *fp)
{
	fp->f_flags &= ~(_F_EOF|_F_ERR);
}

/*
 * setbuffer()
 *	Set buffer with given size
 */
void
setbuffer(FILE *fp, char *buf, uint size)
{
	if (buf == 0) {
		fp->f_flags |= _F_UNBUF;
		return;
	}
	fp->f_pos = fp->f_buf = buf;
	fp->f_bufsz = size;
	fp->f_cnt = 0;
	fp->f_flags |= _F_UBUF;
}
/*
 * setbuf()
 *	Set buffering for file
 */
void
setbuf(FILE *fp, char *buf)
{
	setbuffer(fp, buf, BUFSIZ);
}

/*
 * puts()
 *	Put a string, add a newline
 */
puts(const char *s)
{
	char c;

	while (c = *s++) {
		if (fputc(c, stdout) == EOF) {
			return(EOF);
		}
	}
	if (fputc('\n', stdout) == EOF) {
		return(EOF);
	}
	return(0);
}

/*
 * fputs()
 *	Put a string, no tailing newline (in the string already, probably)
 */
fputs(const char *s, FILE *fp)
{
	char c;

	while (c = *s++) {
		if (fputc(c, fp) == EOF) {
			return(EOF);
		}
	}
	return(0);
}

/*
 * fwrite()
 *	Write a buffer
 *
 * This could be sped up by noting the remaining buffer space and
 * blasting it all in a single bcopy().  Issues would remain WRT
 * end-of-line handling, etc.
 */
fwrite(const void *buf, int size, int nelem, FILE *fp)
{
	const char *p;
	uint len, x;

	p = buf;
	len = size * nelem;
	x = 0;
	while (x < len) {
		if (fputc(*p, fp)) {
			return(x / size);
		}
		++p; ++x;
	}
	return(nelem);
}

/*
 * fread()
 *	Read a buffer
 */
fread(void *buf, int size, int nelem, FILE *fp)
{
	char *p;
	uint len, x;
	int c;

	p = buf;
	len = size * nelem;
	x = 0;
	while (x < len) {
		c = fgetc(fp);
		if (c == EOF) {
			return(x / size);
		}
		*p++ = c;
		++x;
	}
	return(nelem);
}

/*
 * ungetc()
 *	Push back a character
 */
ungetc(int c, FILE *fp)
{
	/*
	 * Ensure state of buffered file allows for pushback
	 * of data.
	 */
	if ((c == EOF) ||
			!(fp->f_flags & _F_READ) ||
			(fp->f_buf == 0) ||
			(fp->f_pos == fp->f_buf) ||
			(fp->f_flags & (_F_DIRTY|_F_ERR))) {
		return(EOF);
	}

	/*
	 * If he hit EOF, then move back to pre-EOF state
	 */
	fp->f_flags &= ~(_F_EOF);

	/*
	 * Add data to buffer
	 */
	fp->f_pos -= 1;
	(fp->f_pos)[0] = c;
	fp->f_cnt += 1;
	return(c);
}

/*
 * ftell()
 *	Tell position of file
 *
 * Needs to keep in mind both underlying file position and state
 * of buffer.
 */
off_t
ftell(FILE *fp)
{
	long l;

	/*
	 * Get basic position in file
	 */
	l = lseek(fp->f_fd, 0L, SEEK_CUR);

	/*
	 * Dirty buffer--position is file position plus amount
	 * of buffered data.
	 */
	if (fp->f_flags & _F_DIRTY) {
		return(l + fp->f_cnt);
	}

	/*
	 * Clean buffer--position is file position minus amount
	 * buffered but not yet read.
	 */
	return(l - fp->f_cnt);
}

/*
 * fseek()
 *	Set buffered file position
 */
off_t
fseek(FILE *fp, off_t off, int whence)
{
	/*
	 * Clear out any pending dirty data.  Reset buffering so
	 * we will fill from new position.
	 */
	if (fp->f_flags & _F_DIRTY) {
		flushbuf(fp);
	} else {
		fp->f_pos = fp->f_buf;
		fp->f_cnt = 0;
	}

	/*
	 * Flush out any sticky conditions
	 */
	clearerr(fp);

	/*
	 * Let lseek() do the work - we have slightly different return
	 * results however
	 */
	return((lseek(fp->f_fd, off, whence) >= 0) ? 0 : -1);
}

/*
 * rewind()
 *	Set file position to start
 */
void
rewind(FILE *fp)
{
	fseek(fp, (off_t)0, SEEK_SET);
}

/*
 * tmpfile()
 *	Open a tmp file
 */
FILE *
tmpfile(void)
{
	char *buf;

	buf = alloca(32);
	if (buf == 0) {
		return(0);
	}
	sprintf(buf, "/tmp/tf%d", getpid());
	return(fopen(buf, "w"));
}
@


1.17
log
@Add some const declarations
@
text
@a444 1
			m |= _F_WRITE;
d706 1
a706 1
	while ((c = fgetc(fp)) != EOF) {
d710 1
a710 3
		if (x++ < len) {
			*p++ = c;
		}
d957 2
a958 1
	 * Let lseek() do its work and return result
d960 1
a960 1
	return(lseek(fp->f_fd, off, whence));
@


1.16
log
@Create procedural interfaces to all global C library data
@
text
@d408 1
a408 1
fdopen(int fd, char *mode)
d410 2
a411 1
	char *p, c;
d478 1
a478 1
freopen(char *name, char *mode, FILE *fp)
d480 2
a481 1
	char *p, c;
d558 1
a558 1
fopen(char *name, char *mode)
d792 1
a792 1
puts(char *s)
d811 1
a811 1
fputs(char *s, FILE *fp)
d831 1
a831 1
fwrite(void *buf, int size, int nelem, FILE *fp)
d833 1
a833 1
	char *p;
@


1.15
log
@Add tmpfile()
@
text
@d16 1
a16 1
FILE __iob[3] = {
d23 9
d610 1
a610 1
	if ((fp < &__iob[0]) || (fp >= &__iob[3])) {
@


1.14
log
@Allow "+" modifier to give read as well as write
@
text
@d9 1
d962 17
@


1.13
log
@Fix return type of fflush
@
text
@d435 1
d507 2
a508 2
			m |= _F_WRITE;
			o |= O_WRITE;
@


1.12
log
@Add rewind()
@
text
@d610 1
@


1.11
log
@Fix reversed args, which keeps fseek() from working
@
text
@d950 10
@


1.10
log
@Clear EOF/ERR when you reposition file
@
text
@d905 1
a905 1
	l = lseek(fp->f_fd, SEEK_END, 0L);
@


1.9
log
@When adding a character after flushing, need to mark fp dirty
again.  Otherwise the next write thinks it's switching from
read to write with buffered data, and mis-sets the position.
@
text
@d941 5
@


1.8
log
@Add a little fudging for MS-DOS end-of-lines, at least for
our line-oriented routines.
@
text
@d372 1
@


1.7
log
@Add fdopen().  Fiddle setbuf/setbuffer to support both
interfaces.
@
text
@d656 3
d667 3
d693 3
@


1.6
log
@Fiddle stuff around so we can get both fopen() and freopen()
@
text
@d393 68
d476 1
a476 1
	if (fp->f_buf) {
d595 1
a595 1
	if (fp->f_buf) {
d739 2
a740 2
 * setbuf()
 *	Set buffering for file
d743 1
a743 1
setbuf(FILE *fp, char *buf)
d750 1
a750 1
	fp->f_bufsz = BUFSIZ;
d752 10
@


1.5
log
@Add fseek/ftell.  Fix an usigned/signed mistake where an error
value was cast to unsigned before being checked for -1.
@
text
@d393 2
a394 2
 * fopen()
 *	Open a buffered file
d397 1
a397 1
fopen(char *name, char *mode)
a400 1
	FILE *fp;
d403 1
a403 1
	 * Get a file buffer
d405 5
a409 2
	if ((fp = malloc(sizeof(FILE))) == 0) {
		return(0);
a415 1
		free(fp);
a440 1
			free(fp);	/* Or ignore? */
a449 1
		free(fp);
d468 32
@


1.4
log
@Add fread/fwrite
@
text
@d400 1
a400 1
	int m = 0, o = 0;
d448 2
a449 1
	if ((fp->f_fd = open(name, o)) < 0) {
d453 1
d725 1
d736 89
@


1.3
log
@Add puts/fputs
@
text
@d687 47
@


1.2
log
@Add some missing functions
@
text
@d652 35
@


1.1
log
@Initial revision
@
text
@d61 7
d460 1
d494 6
a499 2
	free(fp->f_buf);
	free(fp);
d598 53
@
