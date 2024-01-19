head	1.3;
access;
symbols;
locks; strict;
comment	@# @;


1.3
date	94.10.02.02.43.40;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.10.01.03.35.11;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.08.29.18.08.25;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.3
log
@Add install target
@
text
@CC=gcc
LD=ld
INCS=-I../../../include
CFLAGS=-Wall -DDEBUG $(INCS) -O
LDFLAGS=-L../../../libc
OBJS=main.o ne.o rw.o dir.o stat.o netlib.o
LIBS=-lusr -lc

all: ne

.c.o:
	$(CC) $(CFLAGS) -c $*.c

ne: $(OBJS)
	$(LD) $(LDFLAGS) -o ne ../../../libc/crt0.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f ne

install: ne
	strip ne
	cp ne ../../../boot/ne
@


1.2
log
@Fix makefiles for shared libraries
@
text
@d22 4
@


1.1
log
@Initial revision
@
text
@d15 1
a15 1
	$(LD) $(LDFLAGS) -o ne ../../../libc/crt0srv.o $(OBJS) $(LIBS)
@
