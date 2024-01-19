head	1.9;
access;
symbols
	V1_3_1:1.7
	V1_3:1.7
	V1_2:1.6
	V1_1:1.6
	V1_0:1.3;
locks; strict;
comment	@# @;


1.9
date	95.01.10.05.17.18;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	94.10.01.03.34.46;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	94.04.06.21.55.36;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.11.16.02.48.52;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.09.28.19.35.18;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.08.19.04.14.18;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.07.09.18.34.54;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.03.26.23.31.19;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.12.34;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.9
log
@Leave out -g by default
@
text
@CC= gcc
LD= ld
INCS= -I../../include
CFLAGS= -DDEBUG $(INCS)
OBJS= main.o rw.o swap.o stat.o
LIBS= -L../../libc -lusr -lc_s

.c.o:
	$(CC) $(CFLAGS) -c $*.c

swap: $(OBJS)
	$(LD) -o swap ../../libc/crt0srv.o $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f swap

install: swap
	strip swap
	cp swap ../../boot
@


1.8
log
@Fix makefiles for shared libraries
@
text
@d4 1
a4 1
CFLAGS= -DDEBUG $(INCS) -g
@


1.7
log
@Add clean, clobber, install targets
@
text
@d6 1
a6 1
LIBS= -L../../libc -lusr -lc
@


1.6
log
@Source reorg
@
text
@d6 1
a6 1
LIBS= -L../../libc -L../../lib -lc -lusr
d15 8
a22 2
	del *.o
	del swap
@


1.5
log
@Use del, not rm
@
text
@d3 1
a3 1
INCS= -I../include -I..
d6 1
a6 1
LIBS= -L../libc -L../lib -lc -lusr
d12 1
a12 1
	$(LD) -o swap ../libc/crt0srv.o $(OBJS) $(LIBS)
@


1.4
log
@Fix make clean
@
text
@d15 2
a16 1
	rm -f *.o swap
@


1.3
log
@Boot args work
@
text
@d13 3
@


1.2
log
@Convert to current build environment
@
text
@d12 1
a12 1
	$(LD) -o swap ../libc/crt0.o $(OBJS) $(LIBS)
@


1.1
log
@Initial revision
@
text
@d2 2
a3 1
INCS= -I..
d6 1
a6 1
LIBS= ../msgfake/libc.a ../lib/libusr.a
d12 1
a12 1
	$(CC) $(CFLAGS) -o swap $(OBJS) $(LIBS)
@
