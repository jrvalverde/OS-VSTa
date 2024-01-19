head	1.11;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.8
	V1_1:1.8
	V1_0:1.5;
locks; strict;
comment	@# @;


1.11
date	94.10.05.23.26.56;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.10.01.03.35.11;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.06.21.55.34;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.45.04;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.09.28.19.35.45;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.19.04.15.00;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.07.09.18.33.59;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.04.02.21.50.12;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.08.19.44.14;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.05.16.05.29;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.47.32;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.11
log
@Use optimizer
@
text
@CC= gcc
LD= ld
INCS= -I../../../include
CFLAGS= -DDEBUG $(INCS) -O
LDFLAGS= -L../../../libc
OBJS= main.o rw.o stat.o dir.o
LIBS= -lusr -lc

all: fd
.c.o:
	$(CC) $(CFLAGS) -c $*.c

fd: $(OBJS)
	$(LD) $(LDFLAGS) -o fd ../../../libc/crt0.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f fd

install: all
	strip fd
	cp fd ../../../boot
@


1.10
log
@Fix makefiles for shared libraries
@
text
@d4 1
a4 1
CFLAGS= -DDEBUG $(INCS) -g
@


1.9
log
@Add clean, clobber, install targets
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -o fd ../../../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.8
log
@Source reorg
@
text
@d5 1
a5 1
LDFLAGS= -L../../../libc -L../../../lib
d17 8
a24 2
	del *.o
	del fd
@


1.7
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
	$(LD) $(LDFLAGS) -o fd ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.6
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o fd
@


1.5
log
@Boot args work
@
text
@d15 3
@


1.4
log
@namer stuff in libc
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -o fd ../libc/crt0.o $(OBJS) $(LIBS)
@


1.3
log
@Fix makefile for build
@
text
@d7 1
a7 1
LIBS= ../namer/libnamer.a -lc -lusr
@


1.2
log
@Switch Makefile to use our C library and headers
@
text
@d7 1
a7 1
LIBS= -lc -lusr
@


1.1
log
@Initial revision
@
text
@d2 2
a3 1
INCS= -I..
d5 1
d7 1
a7 1
LIBS= ../msgfake/libc.a ../lib/libusr.a -lpc
d9 1
d14 1
a14 1
	$(CC) $(CFLAGS) -o fd $(OBJS) $(LIBS)
@
