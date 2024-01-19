head	1.4;
access;
symbols;
locks; strict;
comment	@# @;


1.4
date	94.10.01.03.35.11;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	94.06.03.04.47.16;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	94.05.30.21.27.12;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	94.05.30.04.21.59;	author vandys;	state Exp;
branches;
next	;


desc
@@


1.4
log
@Fix makefiles for shared libraries
@
text
@CC	= gcc
LD	= ld
RM	= rm -f
INCS	= -I../../../include
CFLAGS	= -Wall $(INCS)
LDFLAGS	= -L../../../libc
OBJS	= main.o rw.o dir.o stat.o
CLIB	= ../../../libc/libc.a
LIBS	= -lusr -lc
CRT0	= ../../../libc/crt0.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: nvram

nvram: $(OBJS)
	$(LD) $(LDFLAGS) -o nvram $(CRT0) $(OBJS) $(LIBS)

clean:
	$(RM) *.o

clobber: clean
	$(RM) nvram

install: nvram
	strip nvram
	cp nvram ../../../boot
@


1.3
log
@Add make install
@
text
@d10 1
a10 1
CRT0	= ../../../libc/crt0srv.o
@


1.2
log
@Syslog support, bug fix
@
text
@d25 4
@


1.1
log
@Initial revision
@
text
@d5 2
a6 2
CFLAGS	= -Wall $(INCS) -O2
LDFLAGS	= -L../../../libc -L../../../lib
d23 1
a23 1
realclean: clean
@
