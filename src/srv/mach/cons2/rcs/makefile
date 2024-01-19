head	1.10;
access;
symbols
	V1_3_1:1.9
	V1_3:1.9
	V1_2:1.6
	V1_1:1.6
	V1_0:1.3;
locks; strict;
comment	@# @;


1.10
date	94.10.01.03.35.11;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	94.04.07.00.48.59;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.04.06.21.55.33;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.02.28.19.16.48;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.45.50;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.09.28.19.35.45;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.19.04.15.00;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.07.09.18.33.48;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.18.16.38;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.42.57;	author vandys;	state Exp;
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
OBJS= main.o cons.o stat.o isr.o rw.o
LIBS= -lusr -lc_s

.c.o:
	$(CC) $(CFLAGS) -c $*.c

cons: $(OBJS)
	$(LD) $(LDFLAGS) -e start -o cons ../../../libc/crt0srv.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f cons

install: cons
	strip cons
	cp cons ../../../boot
@


1.9
log
@Add support for -mono and -color
@
text
@d7 1
a7 1
LIBS= -lusr -lc
@


1.8
log
@Add clean, clobber, install targets
@
text
@d4 1
a4 1
CFLAGS= -Wall -DCGA -DDEBUG $(INCS) -O
@


1.7
log
@Add files, use -Wall
@
text
@d5 1
a5 1
LDFLAGS= -L../../../libc -L../../../lib
d16 8
a23 2
	del *.o
	del cons
@


1.6
log
@Source reorg
@
text
@d4 1
a4 1
CFLAGS= -DCGA -DDEBUG $(INCS) -O
d6 1
a6 1
OBJS= main.o cons.o stat.o
@


1.5
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
d13 1
a13 1
	$(LD) $(LDFLAGS) -e start -o cons ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.4
log
@Fix make clean
@
text
@d16 2
a17 1
	rm -f *.o cons
@


1.3
log
@Boot args work
@
text
@d14 3
@


1.2
log
@Start using optmizer
@
text
@d13 1
a13 1
	$(LD) $(LDFLAGS) -e start -o cons ../libc/crt0.o $(OBJS) $(LIBS)
@


1.1
log
@Initial revision
@
text
@d4 1
a4 1
CFLAGS= -DCGA -DDEBUG $(INCS) -g
@
