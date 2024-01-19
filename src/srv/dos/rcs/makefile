head	1.11;
access;
symbols
	V1_3_1:1.10
	V1_3:1.10
	V1_2:1.7
	V1_1:1.7
	V1_0:1.4;
locks; strict;
comment	@# @;


1.11
date	94.10.01.03.34.46;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	94.04.09.02.53.51;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.06.21.55.32;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.03.23.21.58.39;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.48.09;	author vandys;	state Exp;
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
date	93.07.09.18.35.38;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.02.21.50.37;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.18.17.57;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.02.19.15.37.47;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.11
log
@Fix makefiles for shared libraries
@
text
@CC= gcc
LD= ld
INCS= -I../../include -I../..
CFLAGS= -Wall -DDEBUG $(INCS) -O
LDFLAGS= -L../../libc
OBJS= main.o fat.o block.o node.o dir.o open.o rw.o stat.o
LIBS= -lusr -lc_s

all: dos
.c.o:
	$(CC) $(CFLAGS) -c $*.c

dos: $(OBJS)
	$(LD) $(LDFLAGS) -o dos ../../libc/crt0srv.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f dos

install: all
	strip dos
	cp dos ../../boot
@


1.10
log
@Do clean under clobber
@
text
@d7 1
a7 1
LIBS= -lusr -lc
@


1.9
log
@Add clean, clobber, install targets
@
text
@d19 1
a19 1
clobber:
@


1.8
log
@Turn on -Wall
@
text
@d5 1
a5 1
LDFLAGS= -L../../libc -L../../lib
d17 8
a24 2
	del *.o
	del dos
@


1.7
log
@Source reorg
@
text
@d4 1
a4 1
CFLAGS= -DDEBUG $(INCS) -O
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
	$(LD) $(LDFLAGS) -o dos ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.5
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o dos
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
	$(LD) $(LDFLAGS) -o dos ../libc/crt0.o $(OBJS) $(LIBS)
@


1.2
log
@Start using optimizer
@
text
@d7 1
a7 1
LIBS= ../namer/libnamer.a -lusr -lc
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
