head	1.30;
access;
symbols
	V1_3_1:1.24
	V1_3:1.24
	V1_2:1.15
	V1_1:1.15
	V1_0:1.8;
locks; strict;
comment	@# @;


1.30
date	94.12.21.05.38.30;	author vandys;	state Exp;
branches;
next	1.29;

1.29
date	94.10.12.04.03.03;	author vandys;	state Exp;
branches;
next	1.28;

1.28
date	94.09.30.22.54.54;	author vandys;	state Exp;
branches;
next	1.27;

1.27
date	94.07.10.18.25.32;	author vandys;	state Exp;
branches;
next	1.26;

1.26
date	94.07.10.17.42.51;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	94.05.22.18.43.41;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	94.04.07.05.00.50;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	94.04.07.00.11.57;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	94.04.06.21.55.20;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	94.03.30.17.58.19;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	94.03.23.21.53.48;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	94.03.15.22.08.41;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	94.03.15.22.07.30;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	94.03.07.17.51.22;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	94.03.01.17.19.49;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.11.19.03.34.40;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.10.23.21.20.09;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.10.17.19.27.21;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.10.09.03.22.14;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.10.08.02.26.57;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.09.28.19.36.27;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.08.19.04.15.21;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.05.03.21.32.29;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.03.26.23.34.14;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.25.21.31.04;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.24.00.39.02;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.03.11.19.20.32;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.26.18.45.04;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.25.21.23.06;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.16.18.50;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.30
log
@Obsolete this version of the "ps" command
@
text
@CC= gcc
LD= ld
CFLAGS= -O -I../../include -I../..
LDFLAGS= -L../../libc
LIBS= -lc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

#
# Note: "ps", which used to be here, is now located in srv/proc/cmd
#
PROGS=test args cat id ls swapd rm cp sleep mkdir pwd kill mv stat \
	dumpsect man fstab rmdir touch

all: $(PROGS)

touch: touch.o
	$(LD) $(LDFLAGS) -o touch ../../libc/crt0.o touch.o -lc

rmdir: rmdir.o
	$(LD) $(LDFLAGS) -o rmdir ../../libc/crt0.o rmdir.o -lc

fstab: fstab.o
	$(LD) $(LDFLAGS) -o fstab ../../libc/crt0.o fstab.o -lc

man: man.o
	$(LD) $(LDFLAGS) -o man ../../libc/crt0.o man.o -lc

stat: stat.o
	$(LD) $(LDFLAGS) -o stat ../../libc/crt0.o stat.o -lc

dump.o: ../testsh/dump.c
	$(CC) $(CFLAGS) -c ../testsh/dump.c

dumpsect: dumpsect.o dump.o
	$(LD) $(LDFLAGS) -o dumpsect ../../libc/crt0.o dumpsect.o dump.o -lc

mv: mv.o
	$(LD) $(LDFLAGS) -o mv ../../libc/crt0.o mv.o -lc

pwd: pwd.o
	$(LD) $(LDFLAGS) -o pwd ../../libc/crt0.o pwd.o -lc

kill: kill.o
	$(LD) $(LDFLAGS) -o kill ../../libc/crt0.o kill.o -lc

mkdir: mkdir.o
	$(LD) $(LDFLAGS) -o mkdir ../../libc/crt0.o mkdir.o -lc

sleep: sleep.o
	$(LD) $(LDFLAGS) -o sleep ../../libc/crt0.o sleep.o -lc

cp: cp.o
	$(LD) $(LDFLAGS) -o cp ../../libc/crt0.o cp.o -lc

rm: rm.o
	$(LD) $(LDFLAGS) -o rm ../../libc/crt0.o rm.o -lc

swapd: swapd.o
	$(LD) $(LDFLAGS) -o swapd ../../libc/crt0.o swapd.o -lc

ls: ls.o
	$(LD) $(LDFLAGS) -o ls ../../libc/crt0.o ls.o -lusr -lc

id: id.o
	$(LD) $(LDFLAGS) -o id ../../libc/crt0.o id.o -lc

cat: cat.o
	$(LD) $(LDFLAGS) -o cat ../../libc/crt0.o cat.o -lc

test: test.o operator.o
	$(LD) $(LDFLAGS) -o test ../../libc/crt0.o test.o operator.o -lc

args: args.o
	$(LD) $(LDFLAGS) -o args ../../libc/crt0.o args.o -lc

clean:
	rm -f *.o

clobber: clean
	rm -f $(PROGS)

install: all
	strip $(PROGS)
	cp $(PROGS) ../../bin
@


1.29
log
@Use dumping code from testsh
@
text
@d10 3
d14 1
a14 1
	dumpsect man fstab ps rmdir touch
a22 3

ps: ps.o
	$(LD) $(LDFLAGS) -o ps ../../libc/crt0.o ps.o -lc
@


1.28
log
@Add touch
@
text
@d33 5
a37 2
dumpsect: dumpsect.o
	$(LD) $(LDFLAGS) -o dumpsect ../../libc/crt0.o dumpsect.o -lc
@


1.27
log
@Add rmdir
@
text
@d11 1
a11 1
	dumpsect man fstab ps rmdir
d14 3
@


1.26
log
@Add ps
@
text
@d11 1
a11 1
	dumpsect man fstab ps
d14 3
@


1.25
log
@Add fstab
@
text
@d11 1
a11 1
	dumpsect man fstab
d14 3
@


1.24
log
@Add man(1)
@
text
@d11 1
a11 1
	dumpsect man
d14 3
@


1.23
log
@use rm
@
text
@d10 2
a11 1
PROGS=test args cat id ls swapd rm cp sleep mkdir pwd kill mv stat dumpsect
d14 3
@


1.22
log
@Add clean, clobber, install targets
@
text
@d60 1
a60 1
	del *.o
@


1.21
log
@Convert to "real" test command ala UNIX
@
text
@d4 1
a4 1
LDFLAGS= -L../../libc -L../../lib
d10 1
a10 1
all: test args cat id ls swapd rm cp sleep mkdir pwd kill mv stat dumpsect
d12 2
d61 7
a67 13
	del test
	del ls
	del cat
	del args
	del swapd
	del id
	del rm
	del cp
	del mkdir
	del sleep
	del pwd
	del kill
	del mv
@


1.20
log
@Add stat/dumpsect
@
text
@d51 2
a52 2
test: test.o
	$(LD) $(LDFLAGS) -o test ../../libc/crt0.o test.o -lc
@


1.19
log
@Add make clean for mv
@
text
@d10 7
a16 1
all: test args cat id ls swapd rm cp sleep mkdir pwd kill mv
@


1.18
log
@Add mv.c/mv command
@
text
@d65 1
@


1.17
log
@Add pwd/kill
@
text
@d10 4
a13 1
all: test args cat id ls swapd rm cp sleep mkdir pwd kill
@


1.16
log
@Forgot to clean up sleep
@
text
@d10 1
a10 1
all: test args cat id ls swapd rm cp sleep mkdir
d12 6
d19 1
a19 1
	$(LD) $(LDFLAGS) -e start -o mkdir ../../libc/crt0.o mkdir.o -lc
d22 1
a22 1
	$(LD) $(LDFLAGS) -e start -o sleep ../../libc/crt0.o sleep.o -lc
d25 1
a25 1
	$(LD) $(LDFLAGS) -e start -o cp ../../libc/crt0.o cp.o -lc
d28 1
a28 1
	$(LD) $(LDFLAGS) -e start -o rm ../../libc/crt0.o rm.o -lc
d31 1
a31 1
	$(LD) $(LDFLAGS) -e start -o swapd ../../libc/crt0.o swapd.o -lc
d34 1
a34 1
	$(LD) $(LDFLAGS) -e start -o ls ../../libc/crt0.o ls.o -lusr -lc
d37 1
a37 1
	$(LD) $(LDFLAGS) -e start -o id ../../libc/crt0.o id.o -lc
d40 1
a40 1
	$(LD) $(LDFLAGS) -e start -o cat ../../libc/crt0.o cat.o -lc
d43 1
a43 1
	$(LD) $(LDFLAGS) -e start -o test ../../libc/crt0.o test.o -lc
d46 1
a46 1
	$(LD) $(LDFLAGS) -e start -o args ../../libc/crt0.o args.o -lc
d60 2
@


1.15
log
@Add mkdir
@
text
@d53 1
@


1.14
log
@New files rm/cp to cleanup
@
text
@d10 1
a10 1
all: test args cat id ls swapd rm cp sleep
d12 3
d52 1
@


1.13
log
@Add sleep command
@
text
@d47 2
@


1.12
log
@Use -O
@
text
@d10 4
a13 1
all: test args cat id ls swapd rm cp
@


1.11
log
@Add rm and cp
@
text
@d3 1
a3 1
CFLAGS= -g -I../../include -I../..
@


1.10
log
@Use del, not rm
@
text
@d10 7
a16 1
all: test args cat id ls swapd
@


1.9
log
@Fix make clean
@
text
@d31 7
a37 1
	rm -f *.o test ls cat args swapd id
@


1.8
log
@Add -lusr for "ls"
@
text
@d31 1
a31 1
	rm -f test test.o
@


1.7
log
@Add swapd
@
text
@d3 2
a4 2
CFLAGS= -O -I../../include -I../..
LDFLAGS= -L../../libc
d16 1
a16 1
	$(LD) $(LDFLAGS) -e start -o ls ../../libc/crt0.o ls.o -lc
@


1.6
log
@New ls command
@
text
@d3 1
a3 1
CFLAGS= -I../../include -I../..
d10 4
a13 1
all: test args cat id ls
@


1.5
log
@Add "id" command
@
text
@d10 4
a13 1
all: test args cat id
@


1.4
log
@Fix for new src dir location
@
text
@d10 4
a13 1
all: test args cat
@


1.3
log
@Add cat(1) to standard build
@
text
@d3 2
a4 2
CFLAGS= -I../include -I..
LDFLAGS= -L../libc
d13 1
a13 1
	$(LD) $(LDFLAGS) -e start -o cat ../libc/crt0.o cat.o -lc
d16 1
a16 1
	$(LD) $(LDFLAGS) -e start -o test ../libc/crt0.o test.o -lc
d19 1
a19 1
	$(LD) $(LDFLAGS) -e start -o args ../libc/crt0.o args.o -lc
@


1.2
log
@Add args to standard build
@
text
@d10 4
a13 1
all: test args
@


1.1
log
@Initial revision
@
text
@d10 1
a10 1
all: test
d14 3
@
