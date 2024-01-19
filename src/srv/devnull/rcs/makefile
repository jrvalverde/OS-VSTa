head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2;
locks; strict;
comment	@# @;


1.4
date	94.05.30.21.28.50;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.06.21.55.32;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.11.16.02.47.22;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.10.17.18.47.11;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.4
log
@Syslog support plus makefile cleanup
@
text
@CC= gcc
LD= ld
INCS= -I../../include
CFLAGS= -DDEBUG $(INCS) -O
LDFLAGS= -L../../libc
OBJS= main.o
LIBS= -lusr -lc

all: devnull
.c.o:
	$(CC) $(CFLAGS) -c $*.c

devnull: $(OBJS)
	$(LD) $(LDFLAGS) -o devnull ../../libc/crt0.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f devnull

install: all
	strip devnull
	cp devnull ../../boot
@


1.3
log
@Add clean, clobber, install targets
@
text
@d5 1
a5 1
LDFLAGS= -L../../libc -L../../lib
@


1.2
log
@Source reorg
@
text
@d17 8
a24 2
	del *.o
	del devnull
@


1.1
log
@Initial revision
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
	$(LD) $(LDFLAGS) -o devnull ../libc/crt0.o $(OBJS) $(LIBS)
@
