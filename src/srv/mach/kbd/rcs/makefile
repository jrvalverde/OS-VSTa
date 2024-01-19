head	1.8;
access;
symbols
	V1_3_1:1.8
	V1_3:1.8
	V1_2:1.6
	V1_1:1.6
	V1_0:1.3;
locks; strict;
comment	@# @;


1.8
date	94.04.06.21.55.34;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.02.28.19.22.14;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.46.09;	author vandys;	state Exp;
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
date	93.07.09.18.35.17;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.17.18.16.49;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.49.51;	author vandys;	state Exp;
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
@CC= gcc
LD= ld
INCS= -I../../../include
CFLAGS= -DKBD -DDEBUG $(INCS) -O
LDFLAGS= -L../../../libc
OBJS= main.o rw.o isr.o stat.o
LIBS= -lusr -lc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

kbd: $(OBJS)
	$(LD) $(LDFLAGS) -e start -o kbd ../../../libc/crt0srv.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f kbd

install: kbd
	strip kbd
	cp kbd ../../../boot
@


1.7
log
@Add KBD define so mach/kbd.h can be shared
@
text
@d5 1
a5 1
LDFLAGS= -L../../../libc -L../../../lib
a6 1
CLIB=../../../libc/libc.a
d16 8
a23 2
	del *.o
	del kbd
@


1.6
log
@Source reorg
@
text
@d4 1
a4 1
CFLAGS= -DDEBUG $(INCS) -O
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
d7 1
a7 1
CLIB=../libc/libc.a
d14 1
a14 1
	$(LD) $(LDFLAGS) -e start -o kbd ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.4
log
@Fix make clean
@
text
@d17 2
a18 1
	rm -f *.o kbd
@


1.3
log
@Boot args work
@
text
@d15 3
@


1.2
log
@Start using optimizer
@
text
@d14 1
a14 1
	$(LD) $(LDFLAGS) -e start -o kbd ../libc/crt0.o $(OBJS) $(LIBS)
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
