head	1.14;
access;
symbols
	V1_3_1:1.12
	V1_3:1.12
	V1_2:1.11
	V1_1:1.10
	V1_0:1.7;
locks; strict;
comment	@# @;


1.14
date	94.10.02.02.26.20;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	94.10.01.03.33.47;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	94.04.06.21.55.20;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	94.01.15.22.45.22;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.09.28.19.36.27;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.08.26.16.38.08;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.08.19.04.15.21;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.04.22.05.10.27;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.25.21.30.35;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.24.17.43.30;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.17.18.18.10;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.03.11.19.20.13;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.10.18.11.17;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.21.24;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.14
log
@Remote unneeded "-e start", fix up -I stuff
@
text
@CC= gcc
LD= ld
INCS=-nostdinc -I../../include
# Add -DSTAND for a more standalone-oriented flavor of shell
CFLAGS= -DSTAND -O $(INCS)
# CFLAGS= -O $(INCS)
LDFLAGS= -L../../libc
LIBS= -lc_s
# LIBS= -lc
CRT0=../../libc/crt0srv.o
# CRT0=../../libc/crt0.o
OBJS= testsh.o dump.o run.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: testsh

testsh: $(OBJS)
	$(LD) $(LDFLAGS) -o testsh $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f testsh

# Copy to ../../bin if not STAND, but we don't seem to use that much
install: all
	strip testsh
	cp testsh ../../boot
@


1.13
log
@Build static/standalone under shlib environment
@
text
@d3 1
d5 2
a6 2
CFLAGS= -DSTAND -O -I../../include -I../..
# CFLAGS= -O -I../../include -I../..
d20 1
a20 1
	$(LD) $(LDFLAGS) -e start -o testsh $(CRT0) $(OBJS) $(LIBS)
@


1.12
log
@Add clean, clobber, install targets
@
text
@d5 1
d7 4
a10 1
LIBS= -lc
d19 1
a19 1
	$(LD) $(LDFLAGS) -e start -o testsh ../../libc/crt0srv.o $(OBJS) -lc
@


1.11
log
@Correct typo
@
text
@d18 9
a26 2
	del testsh
	del *.o
@


1.10
log
@Use del, not rm
@
text
@d3 1
a3 1
# Add -DSTAND for a more standlone-oriented flavor of shell
@


1.9
log
@Default to STAND; non-STAND would use the rc shell
@
text
@d18 2
a19 1
	rm -f testsh *.o
@


1.8
log
@Fix make clean
@
text
@d4 1
a4 1
CFLAGS= -O -I../../include -I../..
d15 1
a15 1
	$(LD) $(LDFLAGS) -e start -o testsh ../../libc/crt0.o $(OBJS) -lc
@


1.7
log
@Get rid of standsh (never used it, anyway).  Use optimizer.
@
text
@d16 3
@


1.6
log
@Build both standalone and interactive versions of shell
@
text
@d3 2
a4 1
CFLAGS= -g -I../../include -I../..
d12 1
a12 1
all: testsh standsh
a15 6

standsh: standsh.o dump.o run.o
	$(LD) $(LDFLAGS) -e start -o standsh ../../libc/crt0.o standsh.o dump.o run.o -lc

standsh.o: testsh.c
	$(CC) -DSTAND $(CFLAGS) -o standsh.o -c testsh.c
@


1.5
log
@Move execution to new file, run.c
@
text
@d11 1
a11 1
all: testsh
d15 6
@


1.4
log
@Start using optimizer
@
text
@d3 1
a3 1
CFLAGS= -O -I../../include -I../..
d6 1
a6 1
OBJS= testsh.o dump.o
@


1.3
log
@Fix paths for testsh's new source dir location
@
text
@d3 1
a3 1
CFLAGS= -I../../include -I../..
@


1.2
log
@Add formatted dump source file
@
text
@d3 2
a4 2
CFLAGS= -I../include -I..
LDFLAGS= -L../libc
d14 1
a14 1
	$(LD) $(LDFLAGS) -e start -o testsh ../libc/crt0.o $(OBJS) -lc
@


1.1
log
@Initial revision
@
text
@d6 1
d13 2
a14 2
testsh: testsh.o
	$(LD) $(LDFLAGS) -e start -o testsh ../libc/crt0.o testsh.o -lc
@
