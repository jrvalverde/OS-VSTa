head	1.10;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.8
	V1_1:1.8
	V1_0:1.5;
locks; strict;
comment	@# @;


1.10
date	94.10.01.03.40.42;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.06.21.55.36;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.11.16.02.48.35;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.09.28.19.35.18;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.08.19.04.14.18;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.07.09.18.34.45;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.05.25.02.26.18;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.20.00.20.57;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.08.19.42.42;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.11.34;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.10
log
@Use static version of C lib
@
text
@CC= gcc
LD= ld
INCS= -I../../include
CFLAGS= -DDEBUG $(INCS) -O
LDFLAGS= -L../../libc
OBJS= main.o open.o rw.o stat.o
LIBS= -lusr -lc_s

all: namer
.c.o:
	$(CC) $(CFLAGS) -c $*.c

namer: $(OBJS)
	$(LD) $(LDFLAGS) -o namer ../../libc/crt0srv.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f namer

install: all
	strip namer
	cp namer ../../boot
@


1.9
log
@Add clean, clobber, install targets
@
text
@d7 1
a7 1
LIBS= -lusr -lc
@


1.8
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
	del namer
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
	$(LD) $(LDFLAGS) -o namer ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.6
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o namer
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
@Optimize
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -o namer ../libc/crt0.o $(OBJS) $(LIBS)
@


1.3
log
@Move namer library into libc
@
text
@d4 1
a4 1
CFLAGS= -DDEBUG $(INCS) -g
@


1.2
log
@Add namer library
@
text
@d7 1
a7 1
LIBS= -lc -lusr
d9 1
a9 1
all: namer libnamer.a
a14 4

libnamer.a: namer.o
	del libnamer.a
	ar rsv libnamer.a namer.o
@


1.1
log
@Initial revision
@
text
@d9 1
a9 1
all: namer
d16 3
a18 2
libname.a: namer.o
	ar ruv libname.a namer.o
@
