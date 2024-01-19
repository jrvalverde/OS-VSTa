head	1.10;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.7
	V1_1:1.7
	V1_0:1.4;
locks; strict;
comment	@# @;


1.10
date	94.10.01.03.35.11;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.06.21.55.35;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.03.04.02.02.21;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.45.20;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.09.28.19.36.27;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.19.04.15.00;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.07.09.18.34.07;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.02.21.50.24;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.18.18.18.49;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.12.19.56.44;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.10
log
@Fix makefiles for shared libraries
@
text
@CC= gcc
LD= ld
INCS= -I../../../include
CFLAGS= -Wall -DDEBUG $(INCS) -O
LDFLAGS= -L../../../libc
OBJS= main.o rw.o wd.o dir.o stat.o
LIBS= -lusr -ldpart -lc_s

all: wd

.c.o:
	$(CC) $(CFLAGS) -c $*.c

wd: $(OBJS)
	$(LD) $(LDFLAGS) -o wd ../../../libc/crt0srv.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f wd

install: all
	strip wd
	cp wd ../../../boot
@


1.9
log
@Add clean, clobber, install targets
@
text
@d7 1
a7 1
LIBS= -lusr -ldpart -lc
@


1.8
log
@Convert to -ldpart
@
text
@d5 1
a5 1
LDFLAGS= -L../../../libc -L../../../lib
d10 1
d18 8
a25 2
	del *.o
	del wd
@


1.7
log
@Source reorg
@
text
@d4 1
a4 1
CFLAGS= -DDEBUG $(INCS) -O
d6 2
a7 2
OBJS= main.o rw.o wd.o fdisk.o dir.o stat.o
LIBS= -lusr -lc
@


1.6
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
	$(LD) $(LDFLAGS) -o wd ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.5
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o wd
@


1.4
log
@Boot args work
@
text
@d15 3
@


1.3
log
@namer stuff in libc
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -o wd ../libc/crt0.o $(OBJS) $(LIBS)
@


1.2
log
@Use optimizer
@
text
@d7 1
a7 1
LIBS= ../namer/libnamer.a -lc -lusr
@


1.1
log
@Initial revision
@
text
@d4 1
a4 1
CFLAGS= -DDEBUG $(INCS) -g
@
