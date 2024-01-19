head	1.1;
access;
symbols;
locks; strict;
comment	@# @;


1.1
date	94.12.19.15.42.13;	author vandys;	state Exp;
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
@CC= gcc
LD= ld
INCS= -I../../include
CFLAGS= -DDEBUG $(INCS) -Wall -O
LDFLAGS= -L../../libc
OBJS= main.o proc.o pstat.o root.o
LIBS= -lusr -lc

all: proc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

proc: $(OBJS)
	$(LD) $(LDFLAGS) -o proc ../../libc/crt0.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f proc

install: all
	strip proc
	cp proc ../../boot
@
