head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1
	V1_2:1.1;
locks; strict;
comment	@ * @;


1.1
date	93.11.25.04.42.37;	author vandys;	state Exp;
branches;
next	;


desc
@FIFO data structure
@


1.1
log
@Initial revision
@
text
@#ifndef FIFO_H
#define FIFO_H
/*
 * fifo.h
 *	A byte-stream FIFO
 */

/*
 * The queue struct
 */
struct fifo {
	int f_size;		/* Size of buf */
	char *f_buf;		/* Data stored here */
	int f_cnt;		/* # bytes */
	int f_hd, f_tl;		/* Head/tail of queue */
};

/*
 * Routines, in-line for speed
 */

/*
 * fifo_put()
 *	Add byte to FIFO
 */
static inline void
fifo_put(struct fifo *f, char c)
{
	if (f->f_cnt >= f->f_size) {
		return;
	}
	f->f_buf[f->f_hd++] = c;
	f->f_cnt += 1;
	if (f->f_hd >= f->f_size) {
		f->f_hd = 0;
	}
}

/*
 * fifo_get()
 *	Take a byte from the FIFO
 */
static inline char
fifo_get(struct fifo *f)
{
	char c;

	if (f->f_cnt == 0) {
		return('\0');
	}
	c = f->f_buf[f->f_tl++];
	f->f_cnt -= 1;
	if (f->f_tl == f->f_size) {
		f->f_tl = 0;
	}
	return(c);
}

/*
 * fifo_empty()
 *	Tell if data in FIFO
 */
static inline int
fifo_empty(struct fifo *f)
{
	return(f->f_cnt == 0);
}

/*
 * External routines
 */
extern struct fifo *fifo_alloc();
extern void fifo_free(struct fifo *);

#endif /* FIFO_H */
@
