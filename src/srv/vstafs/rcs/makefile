head	1.8;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.7
	V1_1:1.7;
locks; strict;
comment	@# @;


1.8
date	94.04.06.21.55.37;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.11.16.02.46.36;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.10.03.20.37.47;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.09.18.17.33.37;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.29.22.26.48;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.29.21.20.25;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.29.18.29.59;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.26.09.00.32;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.8
log
@Add clean, clobber, install targets
@
text
@CC=gcc
LD=ld
INCS=-I../../include
CFLAGS=-DDEBUG -Wall $(INCS) -g
OBJS=main.o alloc.o secio.o buf.o node.o rw.o open.o stat.o
LIBS=-L../../libc -lusr -lc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

vstafs: $(OBJS)
	$(LD) -o vstafs ../../libc/crt0.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f vstafs

install: vstafs
	strip vstafs
	cp vstafs ../../boot
@


1.7
log
@Source reorg
@
text
@d6 1
a6 1
LIBS=-L../../libc -L../../lib -lusr -lc
d15 8
a22 2
	del *.o
	del vstafs
@


1.6
log
@Add clean
@
text
@d3 1
a3 1
INCS=-I../include -I..
d6 1
a6 1
LIBS=-L../libc -L../lib -lusr -lc
d12 1
a12 1
	$(LD) -o vstafs ../libc/crt0.o $(OBJS) $(LIBS)
@


1.5
log
@Add -DDEBUG
@
text
@d13 4
@


1.4
log
@Clean up -Wall warnings
@
text
@d4 1
a4 1
CFLAGS=-Wall $(INCS) -g
@


1.3
log
@Add stat.o
@
text
@d4 1
a4 1
CFLAGS=$(INCS) -g
@


1.2
log
@VSTa-ize compile
@
text
@d5 1
a5 1
OBJS=main.o alloc.o secio.o buf.o node.o rw.o open.o
@


1.1
log
@Initial revision
@
text
@d2 5
a6 2
CFLAGS=-I.. -Ivsta -Ivsta/include -g
OBJS=main.o alloc.o secio.o buf.o node.o rw.o
d8 3
d12 1
a12 1
	$(CC) $(CFLAGS) -o vstafs $(OBJS)
@
