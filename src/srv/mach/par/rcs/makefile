head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3;
locks; strict;
comment	@# @;


1.3
date	94.04.06.21.55.35;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.03.23.21.54.35;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.03.22.18.17.09;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.3
log
@Add clean, clobber, install targets
@
text
@CC=gcc
LD=ld
LIBS=-L../../../libc
CFLAGS=-O -I../../../include
OBJS=main.o par_port.o rw.o stat.o
CRT0=../../../libc/crt0.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: par

par: $(OBJS)
	$(LD) -o par $(CRT0) $(OBJS) $(LIBS) -lusr -lc

lpcp: lpcp.o
	$(LD) -o lpcp $(CRT0) lpcp.o $(LIBS) -lc

clean:
	rm -f *.o

clobber: clean
	rm -f par lpcp

install: all
	strip par
	cp par ../../../boot
@


1.2
log
@Tidy up makefile
@
text
@d2 3
a4 2
VSTADIR=../../..
CFLAGS=-O -I$(VSTADIR)/include -nostdinc
d6 1
d11 1
a11 1
all: par lpcp
d14 1
a14 2
	$(CC) -nostdlib @@objs -o par \
		-L$(VSTADIR)/lib -lusr -L$(VSTADIR)/libc -lc
d17 1
a17 2
	$(CC) -nostdlib $(VSTADIR)/libc/crt0.o lpcp.o \
		-o lpcp -L$(VSTADIR)/libc -lc
d20 8
a27 4
	del *.o
	del par
	del lpcp
	del objs
@


1.1
log
@Initial revision
@
text
@d2 1
a2 1
VSTADIR=../../../vsta
d11 1
a11 1
par: $(OBJS) objs
a14 4
objs: makefile
	echo $(VSTADIR)/libc/crt0.o > objs
	echo $(OBJS) >> objs

a17 6

main.o: main.c par.h par_port.h
par_port.o: par_port.c par_port.h
rw.o: rw.c par.h par_port.h
stat.o: stat.c par.h par_port.h
lpcp.o: lpcp.c
@
