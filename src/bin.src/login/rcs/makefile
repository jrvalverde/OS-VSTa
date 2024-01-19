head	1.7;
access;
symbols
	V1_3_1:1.7
	V1_3:1.6
	V1_2:1.4
	V1_1:1.4
	V1_0:1.2;
locks; strict;
comment	@# @;


1.7
date	94.04.26.21.36.35;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	94.04.07.00.12.41;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	94.04.06.21.55.19;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.09.28.19.36.27;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.08.19.04.15.21;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.05.03.21.33.02;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.03.20.00.24.40;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.7
log
@Add runrc
@
text
@CC= gcc
LD= ld
CFLAGS= -O -I../../include -I../..
LDFLAGS= -L../../libc
LIBS= -lusr -lc
OBJS= login.o
CRT0=../../libc/crt0.o

all: login runrc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

runrc: runrc.o
	$(LD) $(LDFLAGS) -o runrc $(CRT0) runrc.o $(LIBS)

login: $(OBJS)
	$(LD) $(LDFLAGS) -o login $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f login runrc

install: all
	strip login runrc
	cp login runrc ../../bin
@


1.6
log
@use rm, not del
@
text
@d7 1
d9 2
d14 3
d18 1
a18 1
	$(LD) $(LDFLAGS) -e start -o login ../../libc/crt0.o $(OBJS) $(LIBS)
d24 1
a24 1
	rm -f login
d26 3
a28 3
install: login
	strip login
	cp login ../../bin
@


1.5
log
@Add clean, clobber, install targets
@
text
@d15 1
a15 1
	del *.o
d18 1
a18 1
	del login
@


1.4
log
@Use del, not rm
@
text
@d4 1
a4 1
LDFLAGS= -L../../libc -L../../lib
d16 2
d19 4
@


1.3
log
@Fix make clean
@
text
@d15 2
a16 1
	rm -f *.o login
@


1.2
log
@Optimize
@
text
@d13 3
@


1.1
log
@Initial revision
@
text
@d3 1
a3 1
CFLAGS= -g -I../../include -I../..
@
