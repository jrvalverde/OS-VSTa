head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2;
locks; strict;
comment	@# @;


1.4
date	94.05.30.21.26.44;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.06.21.55.35;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.11.25.20.22.44;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.11.25.04.42.37;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.4
log
@Updates for support of RS-232 line control, syslog
@
text
@CC= gcc
LD= ld
INCS= -I../../../include
CFLAGS= -DDEBUG $(INCS) -O
LDFLAGS= -L../../../libc
OBJS= fifo.o main.o rw.o isr.o stat.o control.o
LIBS= -lusr -lc
CRT0= ../../../libc/crt0.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

rs232: $(OBJS)
	$(LD) $(LDFLAGS) -e start -o rs232 $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f rs232

install: rs232
	strip rs232
	cp rs232 ../../../boot
@


1.3
log
@Add clean, clobber, install targets
@
text
@d6 1
a6 1
OBJS= fifo.o main.o rw.o isr.o stat.o
@


1.2
log
@Optimize
@
text
@d5 1
a5 1
LDFLAGS= -L../../../libc -L../../../lib
d17 8
a24 2
	del *.o
	del rs232
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
