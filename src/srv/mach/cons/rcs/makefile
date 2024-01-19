head	1.5;
access;
symbols
	V1_3_1:1.5
	V1_3:1.5
	V1_2:1.4
	V1_1:1.4
	V1_0:1.1;
locks; strict;
comment	@# @;


1.5
date	94.04.06.21.55.33;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.16.02.45.37;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.09.28.19.35.45;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.08.19.04.15.00;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.08.02.20.15.19;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.5
log
@Add clean, clobber, install targets
@
text
@CC        = gcc
LD        = ld
INCLUDES  = -I../../../include
CFLAGS    = -O -DDEBUG $(INCLUDES) $(MACHDEF)

LDFLAGS   = -L../../../libc
LIBS      = -lusr -lc

MACHDEF   = -DIBM_CONSOLE

IBM_OBJS  = main.o con_ibm.o stat.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: cons

cons: $(IBM_OBJS)
	$(LD) $(LDFLAGS) -e start -o cons ../../../libc/crt0srv.o $(IBM_OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f cons

install: all
	strip cons
	cp cons ../../../boot
@


1.4
log
@Source reorg
@
text
@d6 1
a6 1
LDFLAGS   = -L../../../libc -L../../../lib
d22 8
a29 2
	del *.o
	del cons
@


1.3
log
@Use del, not rm
@
text
@d3 1
a3 1
INCLUDES  = -I../include -I..
d6 1
a6 1
LDFLAGS   = -L../libc -L../lib
d19 1
a19 1
	$(LD) $(LDFLAGS) -e start -o cons ../libc/crt0srv.o $(IBM_OBJS) $(LIBS)
@


1.2
log
@Fix make clean
@
text
@d22 2
a23 1
	rm -f -r cons *.o *.c~ *.h~ *.~ 
@


1.1
log
@Initial revision
@
text
@a22 2


@
