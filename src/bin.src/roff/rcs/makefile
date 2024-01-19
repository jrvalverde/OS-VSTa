head	1.1;
access;
symbols
	V1_3_1:1.1
	V1_3:1.1;
locks; strict;
comment	@# @;


1.1
date	94.04.07.00.59.53;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.1
log
@Initial revision
@
text
@CC=gcc
LD=ld
CFLAGS=-O -I../../include
OBJS= roff.o roff1.o itoeng.o
CRT0=../../libc/crt0.o
LIBS=-L../../libc -lc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

roff: $(OBJS)
	$(LD) -o roff $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f roff

install: roff
	strip roff
	cp roff ../../bin
@
