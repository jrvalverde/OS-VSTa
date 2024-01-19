head	1.7;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5
	V1_1:1.5
	V1_0:1.2;
locks; strict;
comment	@# @;


1.7
date	94.10.01.03.34.19;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.04.06.21.55.19;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.43.08;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.09.28.19.36.27;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.19.04.15.21;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.07.09.18.35.46;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.22.22.19.34;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.7
log
@Build init for boot server use
@
text
@CC= gcc
LD= ld
CFLAGS= -O -I../../include
LDFLAGS= -L../../libc
LIBS= -lc_s
OBJS= init.o
CRT0= ../../libc/crt0srv.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: init

init: $(OBJS)
	$(LD) $(LDFLAGS) -e start -o init $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f init

install: all
	strip init
	cp init ../../boot
@


1.6
log
@Add clean, clobber, install targets
@
text
@d5 1
a5 1
LIBS= -lc
d7 1
d15 1
a15 1
	$(LD) $(LDFLAGS) -e start -o init ../../libc/crt0srv.o $(OBJS) -lc
@


1.5
log
@Source reorg
@
text
@d17 8
a24 2
	del *.o
	del init
@


1.4
log
@Use del, not rm
@
text
@d3 1
a3 1
CFLAGS= -O -I../../include -I../..
@


1.3
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o init
@


1.2
log
@Boot args work
@
text
@d15 3
@


1.1
log
@Initial revision
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -e start -o init ../../libc/crt0.o $(OBJS) -lc
@
