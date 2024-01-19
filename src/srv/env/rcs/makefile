head	1.8;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.6
	V1_1:1.6
	V1_0:1.3;
locks; strict;
comment	@# @;


1.8
date	94.10.01.03.34.46;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.06.21.55.33;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.49.12;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.09.28.19.35.45;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.19.04.15.00;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.07.09.18.34.16;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.17.04.56;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.05.16.02.28;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.8
log
@Fix makefiles for shared libraries
@
text
@CC= gcc
LD= ld
INCS= -I../../include
CFLAGS= -DDEBUG $(INCS) -g
LDFLAGS= -L../../libc
OBJS= main.o open.o rw.o stat.o string.o node.o
LIBS= -lusr -lc

all: env
.c.o:
	$(CC) $(CFLAGS) -c $*.c

env: $(OBJS)
	$(LD) $(LDFLAGS) -o env ../../libc/crt0.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f env

install: all
	strip env
	cp env ../../boot
@


1.7
log
@Add clean, clobber, install targets
@
text
@d7 1
a7 1
LIBS= -lc -lusr
d14 1
a14 1
	$(LD) $(LDFLAGS) -o env ../../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.6
log
@Source reorg
@
text
@d5 1
a5 1
LDFLAGS= -L../../libc -L../../lib
d17 8
a24 2
	del *.o
	del env
@


1.5
log
@Use del, not rm
@
text
@d3 1
a3 1
INCS= -I../include -I..
d5 1
a5 1
LDFLAGS= -L../libc -L../lib
d14 1
a14 1
	$(LD) $(LDFLAGS) -o env ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.4
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o env
@


1.3
log
@Boot args work
@
text
@d15 3
@


1.2
log
@Add node handling
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -o env ../libc/crt0.o $(OBJS) $(LIBS)
@


1.1
log
@Initial revision
@
text
@d6 1
a6 1
OBJS= main.o open.o rw.o stat.o string.o
@
