head	1.2;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2
	V1_2:1.1
	V1_1:1.1;
locks; strict;
comment	@# @;


1.2
date	94.04.06.21.55.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.10.01.04.25.57;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.2
log
@Add clean, clobber, install targets
@
text
@CC=gcc
CFLAGS=-I../../include -I../.. -g
LD=ld
CRT0=../../libc/crt0.o
LIB=-L../../libc -lc

PROGS=setime date
all: $(PROGS)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

setime: setime.o time.o getdate.o
	$(LD) $(CRT0) -o setime setime.o time.o getdate.o $(LIB)

date: date.o time.o
	$(LD) $(CRT0) -o date date.o time.o $(LIB)

clean:
	rm -f *.o

clobber: clean
	rm -f $(PROGS)

install: all
	strip $(PROGS)
	cp $(PROGS) ../../bin
@


1.1
log
@Initial revision
@
text
@d7 2
a8 1
all: setime date
d20 8
a27 3
	del *.o
	del setime
	del date
@
