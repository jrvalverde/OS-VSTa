head	1.3;
access;
symbols
	V1_3_1:1.2
	V1_3:1.2;
locks; strict;
comment	@# @;


1.3
date	94.10.01.03.39.44;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.04.06.21.55.34;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.03.22.18.33.08;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.3
log
@Fix library ordering
@
text
@CC	= gcc
LD	= ld
INCS	= -I../../../include
CFLAGS	= -DDEBUG -Wall $(INCS) -O
LDFLAGS	= -L../../../libc
OBJS	= main.o rw.o stat.o
TOBJS	= libjoystick.o jstest.o
LIBS	= -lusr -lpio -lc
CRT0	= ../../../libc/crt0.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

PROGS=joystick jstest
all: $(PROGS)

joystick: $(OBJS)
	$(LD) $(LDFLAGS) -o joystick $(CRT0) $(OBJS) $(LIBS)

jstest: $(TOBJS)
	$(LD) $(LDFLAGS) -o jstest $(CRT0) $(TOBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(PROGS)

install: all
	strip joystick
	cp joystick ../../../boot
@


1.2
log
@Add clean, clobber, install targets
@
text
@d8 1
a8 2
CLIB	= ../../../libc/libc.a
LIBS	= -lusr -lc -lpio
@


1.1
log
@Initial revision
@
text
@a2 1
RM	= del
d5 1
a5 1
LDFLAGS	= -L../../../libc -L../../../lib
d15 2
a16 1
all: joystick jstest
d25 8
a32 3
	$(RM) *.o
	$(RM) joystick
	$(RM) jstest
@
