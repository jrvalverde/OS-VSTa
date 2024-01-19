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
date	94.10.01.03.34.46;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.06.21.55.32;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.03.08.20.04.21;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.47.08;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.09.28.19.35.18;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.08.19.04.14.18;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.07.09.18.33.25;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.04.02.21.48.20;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.10.20.32.19;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.39.15;	author vandys;	state Exp;
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
@CC	= gcc
LD	= ld
INCS	= -I../../include
CFLAGS	= -DDEBUG $(INCS) -Wall -g
LDFLAGS	= -L../../libc
CRT0	= ../../libc/crt0.o
OBJS	= main.o block.o filectrl.o open.o rw.o stat.o
LIBS	= -lusr -lc

all: bfs

.c.o:
	$(CC) $(CFLAGS) -c $*.c

bfs: $(OBJS)
	$(LD) $(LDFLAGS) -o bfs $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f bfs

install: all
	strip bfs
	cp bfs ../../boot
@


1.9
log
@Add clean, clobber, install targets
@
text
@d6 1
a6 1
CRT0	= ../../libc/crt0srv.o
@


1.8
log
@Rev boot filesystem per work from Dave Hudson
@
text
@d5 1
a5 1
LDFLAGS	= -L../../libc -L../../lib
d19 8
a26 2
	del *.o
	del bfs
@


1.7
log
@Source reorg
@
text
@d1 8
a8 7
CC= gcc
LD= ld
INCS= -I../../include
CFLAGS= -DDEBUG $(INCS) -g
LDFLAGS= -L../../libc -L../../lib
OBJS= main.o block.o inode.o dir.o open.o blk.o rw.o stat.o
LIBS= -lusr -lc
d11 1
d16 1
a16 1
	$(LD) $(LDFLAGS) -o bfs ../../libc/crt0srv.o $(OBJS) $(LIBS)
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
	$(LD) $(LDFLAGS) -o bfs ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.5
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o bfs
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
@namer stuff in libc now
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -o bfs ../libc/crt0.o $(OBJS) $(LIBS)
@


1.2
log
@Update build paths and such
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
@d2 2
a3 1
INCS= -I..
d5 1
d7 1
a7 1
LIBS= ../msgfake/libc.a ../lib/libusr.a
d9 1
d14 1
a14 1
	$(CC) $(CFLAGS) -o bfs $(OBJS) $(LIBS)
@
