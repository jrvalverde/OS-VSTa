head	1.6;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5;
locks; strict;
comment	@# @;


1.6
date	94.04.06.21.55.18;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.12.14.23.15.36;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.12.10.19.13.16;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.12.09.06.25.56;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.12.09.06.07.20;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.09.00.26.27;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.6
log
@Add clean, clobber, install targets
@
text
@CC=gcc
LD=ld
CFLAGS=-I../../include -O
OBJS=adb.o disasm.o expr.o lex.o sym.o map.o proc.o i386.o
LIBS=-L../../libc -lc
CRT0=../../libc/crt0.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

adb: $(OBJS)
	$(LD) -o adb $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f adb

install:
	strip adb
	cp adb ../../bin
@


1.5
log
@Optimize
@
text
@d15 8
a22 2
	del *.o
	del adb
@


1.4
log
@Add i386.c
@
text
@d3 1
a3 1
CFLAGS=-I../../include -g
@


1.3
log
@Add make clean
@
text
@d4 1
a4 1
OBJS=adb.o disasm.o expr.o lex.o sym.o map.o proc.o
@


1.2
log
@Add ptrace support
@
text
@d13 4
@


1.1
log
@Initial revision
@
text
@d4 1
a4 1
OBJS=adb.o disasm.o expr.o lex.o sym.o map.o
@
