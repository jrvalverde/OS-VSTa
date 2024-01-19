head	1.4;
access;
symbols
	V1_3_1:1.4
	V1_3:1.4
	V1_2:1.1
	V1_1:1.1
	V1_0:1.1;
locks; strict;
comment	@# @;


1.4
date	94.04.06.21.55.32;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.03.07.18.09.01;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.02.02.19.57.40;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.39.45;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.4
log
@Add clean, clobber, install targets
@
text
@CC	= gcc
LD	= ld
INCS	= -I../../../include
CFLAGS	= $(INCS) -g -Wall
LDFLAGS	= -L../../../libc
CRT0	= ../../../libc/crt0.o
OBJS	= mkbfs.o
LIBS	= -lc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

mkbfs: $(OBJS)
	$(LD) $(LDFLAGS) -o mkbfs $(OBJS) $(CRT0) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f mkbfs

install: mkbfs
	strip mkbfs
	cp mkbfs ../../../bin/mkfs_bfs
@


1.3
log
@Incorporate changes from Dave Hudson
@
text
@d17 8
a24 2
	del *.o
	del mkbfs
@


1.2
log
@Convert to native
@
text
@d1 8
a8 7
CC=gcc
LD=ld
INCS= -I../../../include
CFLAGS= $(INCS) -g
OBJS= mkfs.o
CRT0=../../../libc/crt0.o
LIBS=-L../../../libc -lc
d13 6
a18 2
mkfs: $(OBJS)
	$(LD) -o mkfs $(CRT0) $(OBJS) $(LIBS)
@


1.1
log
@Initial revision
@
text
@d1 3
a3 2
CC= gcc
INCS= -I../..
d6 2
d13 1
a13 1
	$(CC) $(CFLAGS) -o mkfs $(OBJS)
@
