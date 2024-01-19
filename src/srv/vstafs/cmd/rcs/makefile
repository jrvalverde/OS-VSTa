head	1.6;
access;
symbols
	V1_3_1:1.6
	V1_3:1.6
	V1_2:1.5
	V1_1:1.5;
locks; strict;
comment	@# @;


1.6
date	94.04.06.21.55.37;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.11.16.02.46.51;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.11.15.23.28.38;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.11.13.20.02.01;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.10.03.20.38.13;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.09.27.18.57.24;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.6
log
@Add clean, clobber, install targets
@
text
@CC=gcc
LD=ld
INCS=-I../../../include
CFLAGS=$(INCS) -g
LIBS=-L../../../libc -lusr -lc
CRT0=../../../libc/crt0.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

PROGS=mkfs fsdb fsck
all:  $(PROGS)

fsck: fsck.o
	$(LD) -o fsck $(CRT0) fsck.o $(LIBS)

mkfs: mkfs.o
	$(LD) -o mkfs $(CRT0) mkfs.o $(LIBS)

fsdb: fsdb.o
	$(LD) -o fsdb $(CRT0) fsdb.o $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f mkfs fsdb fsck

install: all
	strip $(PROGS)
	cp mkfs ../../../bin/mkfs_vfs
	cp fsdb ../../../bin/fsdb_vfs
	cp fsck ../../../bin/fsck_vfs
@


1.5
log
@Source reorg
@
text
@d5 1
a5 1
LIBS=-L../../../libc -L../../../lib -lusr -lc
d11 2
a12 1
all: mkfs fsdb fsck
d24 10
a33 4
	del *.o
	del mkfs
	del fsdb
	del fsck
@


1.4
log
@Forgot to cleanup fsck
@
text
@d3 1
a3 1
INCS=-I../../include -I../..
d5 2
a6 1
LIBS=-L../../libc -L../../lib -lusr -lc
d14 1
a14 1
	$(LD) -o fsck ../../libc/crt0.o fsck.o $(LIBS)
d17 1
a17 1
	$(LD) -o mkfs ../../libc/crt0.o mkfs.o $(LIBS)
d20 1
a20 1
	$(LD) -o fsdb ../../libc/crt0.o fsdb.o $(LIBS)
@


1.3
log
@Add fsck
@
text
@d25 1
@


1.2
log
@Add clean
@
text
@d10 4
a13 1
all: mkfs fsdb
@


1.1
log
@Initial revision
@
text
@d17 5
@
