head	1.4;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.1;
locks; strict;
comment	@# @;


1.4
date	94.05.30.04.05.17;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.04.06.22.18.12;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.04.06.21.55.34;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.12.27.22.30.12;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.4
log
@Add PS2, convert to syslog, convert to RS-232 server
@
text
@CC        = gcc
LD        = ld
INCLUDES  = -I../../../include
CFLAGS    = -DDEBUG $(INCLUDES) -O

LDFLAGS   = -L../../../libc
LIBS      = -lpio -lusr -lc
CRT0	  = ../../../libc/crt0.o

OBJS      = main.o ibmrs232.o nec_bus.o ms_bus.o logi_bus.o ps2aux.o \
	mouse.o stat.o
TOBJS     = test.o libmouse.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: mouse test

test: $(TOBJS) 
	$(LD) $(LDFLAGS) -e start -o test $(CRT0) $(TOBJS) $(LIBS)

mouse: $(OBJS)
	$(LD) $(LDFLAGS) -e start -o mouse $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f mouse test

clobber: clean
	rm -f *.o

install: all
	strip mouse
	cp mouse ../../../boot
@


1.3
log
@Get rid of @@objs; native VSTa build
@
text
@d10 2
a11 1
OBJS      = main.o ibmrs232.o nec_bus.o ms_bus.o logi_bus.o mouse.o stat.o
@


1.2
log
@Add clean, clobber, install targets
@
text
@d22 1
a22 1
	$(LD) $(LDFLAGS) -e start -o mouse $(CRT0) @@objs $(LIBS)
@


1.1
log
@Initial revision
@
text
@d6 1
a6 1
LDFLAGS   = -L../../../libc -L../../../lib
d25 8
a32 3
	del mouse
	del test
	del *.o
@
