head	1.2;
access;
symbols;
locks; strict;
comment	@# @;


1.2
date	94.12.21.16.47.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.12.19.15.43.38;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.2
log
@Add utils.c
@
text
@CC=gcc
LD=ld
INCS=-I../../../include
CFLAGS=-DDEBUG $(INCS) -Wall -O
LDFLAGS=-L../../../libc
LIBS=-lusr -lc
CRT0=../../../libc/crt0.o

all: kill ps

.c.o:
	$(CC) $(CFLAGS) -c $*.c

UTIL=utils.o

kill: kill.o $(UTIL)
	$(LD) $(LDFLAGS) -o kill $(CRT0) kill.o $(UTIL) $(LIBS)

ps: ps.o $(UTIL)
	$(LD) $(LDFLAGS) -o ps $(CRT0) ps.o $(UTIL) $(LIBS)

install: all
	strip kill
	cp kill /vsta/bin
	strip ps
	cp ps /vsta/bin

clean:
	rm -f *.o

clobber: clean
	rm -f kill ps
@


1.1
log
@Initial revision
@
text
@d1 7
a7 7
CC= gcc
LD= ld
INCS= -I../../../include
CFLAGS= -DDEBUG $(INCS) -Wall -O
LDFLAGS= -L../../../libc
LIBS= -lusr -lc
CRT0= ../../../libc/crt0.o
d14 1
a14 2
kill: kill.o
	$(LD) $(LDFLAGS) -o kill $(CRT0) kill.o $(LIBS)
d16 5
a20 2
ps: ps.o
	$(LD) $(LDFLAGS) -o ps $(CRT0) ps.o $(LIBS)
@
