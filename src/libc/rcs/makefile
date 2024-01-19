head	1.39;
access;
symbols
	V1_3_1:1.32
	V1_3:1.32
	V1_2:1.21
	V1_1:1.21
	V1_0:1.17;
locks; strict;
comment	@# @;


1.39
date	95.01.10.05.22.08;	author vandys;	state Exp;
branches;
next	1.38;

1.38
date	94.10.12.04.00.29;	author vandys;	state Exp;
branches;
next	1.37;

1.37
date	94.10.01.03.31.49;	author vandys;	state Exp;
branches;
next	1.36;

1.36
date	94.09.30.22.54.22;	author vandys;	state Exp;
branches;
next	1.35;

1.35
date	94.09.23.20.38.16;	author vandys;	state Exp;
branches;
next	1.34;

1.34
date	94.09.07.19.17.54;	author vandys;	state Exp;
branches;
next	1.33;

1.33
date	94.07.10.19.16.56;	author vandys;	state Exp;
branches;
next	1.32;

1.32
date	94.04.10.23.41.05;	author vandys;	state Exp;
branches;
next	1.31;

1.31
date	94.04.09.04.35.05;	author vandys;	state Exp;
branches;
next	1.30;

1.30
date	94.04.06.22.18.12;	author vandys;	state Exp;
branches;
next	1.29;

1.29
date	94.04.06.21.55.21;	author vandys;	state Exp;
branches;
next	1.28;

1.28
date	94.04.02.01.53.30;	author vandys;	state Exp;
branches;
next	1.27;

1.27
date	94.03.28.23.19.17;	author vandys;	state Exp;
branches;
next	1.26;

1.26
date	94.03.15.22.05.58;	author vandys;	state Exp;
branches;
next	1.25;

1.25
date	94.02.28.20.02.37;	author vandys;	state Exp;
branches;
next	1.24;

1.24
date	94.02.28.19.14.19;	author vandys;	state Exp;
branches;
next	1.23;

1.23
date	94.02.27.02.29.58;	author vandys;	state Exp;
branches;
next	1.22;

1.22
date	94.02.02.19.41.08;	author vandys;	state Exp;
branches;
next	1.21;

1.21
date	93.11.16.02.50.35;	author vandys;	state Exp;
branches;
next	1.20;

1.20
date	93.10.17.19.25.57;	author vandys;	state Exp;
branches;
next	1.19;

1.19
date	93.09.28.19.35.45;	author vandys;	state Exp;
branches;
next	1.18;

1.18
date	93.08.19.04.14.18;	author vandys;	state Exp;
branches;
next	1.17;

1.17
date	93.07.09.18.36.43;	author vandys;	state Exp;
branches;
next	1.16;

1.16
date	93.04.23.19.48.40;	author vandys;	state Exp;
branches;
next	1.15;

1.15
date	93.04.14.01.11.12;	author vandys;	state Exp;
branches;
next	1.14;

1.14
date	93.04.12.20.56.14;	author vandys;	state Exp;
branches;
next	1.13;

1.13
date	93.04.09.17.12.52;	author vandys;	state Exp;
branches;
next	1.12;

1.12
date	93.03.25.21.29.21;	author vandys;	state Exp;
branches;
next	1.11;

1.11
date	93.03.20.00.22.38;	author vandys;	state Exp;
branches;
next	1.10;

1.10
date	93.03.17.18.16.07;	author vandys;	state Exp;
branches;
next	1.9;

1.9
date	93.03.16.22.22.57;	author vandys;	state Exp;
branches;
next	1.8;

1.8
date	93.03.16.19.06.35;	author vandys;	state Exp;
branches;
next	1.7;

1.7
date	93.03.13.01.31.09;	author vandys;	state Exp;
branches;
next	1.6;

1.6
date	93.03.11.19.17.34;	author vandys;	state Exp;
branches;
next	1.5;

1.5
date	93.03.08.23.04.01;	author vandys;	state Exp;
branches;
next	1.4;

1.4
date	93.02.26.18.43.41;	author vandys;	state Exp;
branches;
next	1.3;

1.3
date	93.02.23.18.19.07;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.02.10.19.09.48;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.59.32;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.39
log
@Don't assume "." is in path
@
text
@CC=gcc
AS=as
LD=ld
CPP= cpp
INCS= -I../include
CFLAGS= $(INCS) -O -DDEBUG
CRT=start.o startsrv.o
OBJS= syscalls.o syscall.o \
	mem.o misc.o io.o setjmp.o malloc.o \
	string.o doprnt.o stdio.o printf.o tty.o ctype.o scanf.o seg.o \
	mount.o open.o stat.o fdl.o hash.o time.o exec.o getopt.o \
	shell.o gnufuncs.o qsort.o float.o signal.o env.o fdmem.o \
	perm.o random.o namer.o dir.o wait.o mktemp.o grp.o pwd.o \
	pipe.o fdnull.o errno.o port.o syslog.o rename.o strsep.o \
	bsearch.o strtol.o strtoul.o swab.o utime.o getline.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

# Libraries to be build
LIBS=libc.a libc_s.a ld.a libusr.a libtermc.a libregex.a libpio.a libdpart.a

# Databases for shared libraries
DBS=ld.db libc.db
SHLIBIN=ld.tmp libc.tmp
SHLIBS=ld.shl libc.shl

all: $(LIBS) crt0.o crt0srv.o

# Statically linkable version of libc
libc_s.a: $(OBJS) $(CRT)
	rm -f libc_s.a
	ar rsv libc_s.a $(OBJS) $(CRT)


# All shared libraries must be processed at once, as mkshlib assigns
# addresses as it processes each.
mklibs: mkshlib $(SHLIBIN) shlib.o
	./mkshlib $(DBS)
	rm -f mklibs
	touch mklibs

# Utility for creating shared libraries
mkshlib: crt0.o mkshlib.o libc_s.a
	$(LD) -o mkshlib crt0.o mkshlib.o libc_s.a

# All shared libraries are created in both .a (stub) and .shl (loadable
# shared library) formats in the same step, "mklibs".
libc.a: mklibs
ld.a: mklibs

# These are the versions of our shared libraries which are used as
# input to mkshlib.  ld.tmp, which must operate as a dynamically
# mapped standalone program, includes a minimal set of system calls
libc.tmp: start.o $(OBJS)
	rm -f libc.tmp
	ld -r -o libc.tmp start.o $(OBJS)

ld.tmp: ld.o shcalls.o
	ld -r -o ld.tmp ld.o shcalls.o

# Special version for boot servers
startsrv.o: start.c
	$(CC) $(CFLAGS) -DSRV -o startsrv.o -c start.c

# Stuff from machine-dependent directory
io.o: mach/io.s
	$(AS) -o io.o mach/io.s
mem.o: mach/mem.s
	$(AS) -o mem.o mach/mem.s
misc.o: mach/misc.s
	$(AS) -o misc.o mach/misc.s
setjmp.o: mach/setjmp.s
	$(AS) -o setjmp.o mach/setjmp.s
# crt0, standard version and one with boot server support
crt0.o: mach/crt0.s
	$(CPP) mach/crt0.s tmp.s
	$(AS) -o crt0.o tmp.s
	rm -f tmp.s
crt0srv.o: mach/crt0.s
	$(CPP) -DSRV mach/crt0.s tmp.s
	$(AS) -o crt0srv.o tmp.s
	rm -f tmp.s
gnufuncs.o: mach/gnufuncs.s
	$(AS) -o gnufuncs.o mach/gnufuncs.s
exec.o: mach/exec.c
	$(CC) $(CFLAGS) -c mach/exec.c

# Our tricky syscall stub which needs to use CPP
syscalls.o: mach/syscalls.s
	$(CPP) -D__ASM__ $(INCS) mach/syscalls.s tmp.s
	as -o syscalls.o tmp.s
	rm -f tmp.s

# Minimal version for shlib loading
shcalls.o: mach/shcalls.s
	$(CPP) -D__ASM__ $(INCS) mach/shcalls.s tmp.s
	as -o shcalls.o tmp.s
	rm -f tmp.s

USROBJS= llist.o hash.o permsup.o permpr.o statsup.o \
	files.o rmap.o passwd.o ids.o assert.o mem.o

libusr.a: $(USROBJS)
	rm -f libusr.a
	ar rsv libusr.a $(USROBJS)

libtermc.a: termcap.o tgoto.o tputs.o
	rm -f libtermc.a
	ar rsv libtermc.a termcap.o tgoto.o tputs.o
termcap.o: term/termcap.c
	$(CC) $(CFLAGS) -c term/termcap.c
tgoto.o: term/tgoto.c
	$(CC) $(CFLAGS) -c term/tgoto.c
tputs.o: term/tputs.c
	$(CC) $(CFLAGS) -c term/tputs.c

libregex.a: regexp.o regsub.o regerror.o
	rm -f libregex.a
	ar rsv libregex.a regexp.o regsub.o regerror.o

regexp.o: regexp/regexp.c
	$(CC) $(CFLAGS) -c regexp/regexp.c
regsub.o: regexp/regsub.c
	$(CC) $(CFLAGS) -c regexp/regsub.c
regerror.o: regexp/regerror.c
	$(CC) $(CFLAGS) -c regexp/regerror.c

libpio.a: pio.o
	rm -f libpio.a
	ar rsv libpio.a pio.o

libdpart.a: dpart.o
	rm -f libdpart.a
	ar rsv libdpart.a dpart.o
dpart.o: mach/dpart.c
	$(CC) $(CFLAGS) -c mach/dpart.c

clean:
	rm -f *.o *.tmp

clobber: clean
	rm -f mklibs *.a *.shl

install: all mklibs
	cp $(LIBS) crt0.o crt0srv.o ../lib
	cp $(SHLIBS) ../lib
@


1.38
log
@Add getline() for canonical input processing.
Fix "make install" to put the .shl's into place.
@
text
@d39 1
a39 1
	mkshlib $(DBS)
@


1.37
log
@Add dependency for crt0.o, make sure mklibs tag is touch'ed
with current date/time
@
text
@d15 1
a15 1
	bsearch.o strtol.o strtoul.o swab.o utime.o
d26 1
d145 1
a145 1
install: all
d147 1
@


1.36
log
@Add shared libraries
@
text
@d39 1
d43 1
a43 1
mkshlib: mkshlib.o libc_s.a
@


1.35
log
@Add creation of shlib C library
@
text
@d1 3
a3 2
CC= gcc
AS= as
d20 7
a26 1
LIBS=libc.a libusr.a libtermc.a libregex.a libpio.a libdpart.a
d29 30
a58 7
libc.a: $(OBJS) $(CRT)
	rm -f libc.a
	ar rsv libc.a $(OBJS) $(CRT)

libc.sh: $(OBJS)
	rm -f libc.sh
	ld -o libc.sh $(OBJS)
d93 6
d138 1
a138 1
	rm -f *.o
d141 1
a141 1
	rm -f *.a
@


1.34
log
@Update bzero()/bcopy() to faster kernel versions, set up so
same routines can be shared by kernel and libc.
@
text
@d6 2
a7 1
OBJS= start.o startsrv.o syscalls.o syscall.o \
d22 1
a22 1
libc.a: $(OBJS)
d24 5
a28 1
	ar rsv libc.a $(OBJS)
@


1.33
log
@Add utime.c
@
text
@d7 1
a7 1
	mem.o io.o setjmp.o malloc.o \
d34 2
d59 1
a59 1
	files.o rmap.o passwd.o ids.o assert.o
@


1.32
log
@Add swab.c
@
text
@d13 1
a13 1
	bsearch.o strtol.o strtoul.o swab.o
@


1.31
log
@Remove extraneous blank line
@
text
@d13 1
a13 1
	bsearch.o strtol.o strtoul.o
@


1.30
log
@Get rid of @@objs; native VSTa build
@
text
@a101 1

@


1.29
log
@Add clean, clobber, install targets
@
text
@d23 1
a23 1
	ar rsv libc.a @@objs
@


1.28
log
@Add strto[u]l.c
@
text
@d18 2
a19 1
all: libc.a crt0.o crt0srv.o
d22 1
a22 1
	del libc.a
d40 1
a40 1
	del tmp.s
d44 1
a44 1
	del tmp.s
d54 1
a54 1
	del tmp.s
d56 2
a57 3
# Share this with the kernel
hash.o: ../lib/hash.c
	$(CC) $(CFLAGS) -c ../lib/hash.c
d59 35
d95 8
a102 2
	del *.o
	del *.a
@


1.27
log
@Add files strsep.c and bsearch.c
@
text
@d13 1
a13 1
	bsearch.o
@


1.26
log
@Add rename.c
@
text
@d12 2
a13 1
	pipe.o fdnull.o errno.o port.o syslog.o rename.o
@


1.25
log
@Add syslog
@
text
@d12 1
a12 1
	pipe.o fdnull.o errno.o port.o syslog.o
@


1.24
log
@Add port.c
@
text
@d12 1
a12 1
	pipe.o fdnull.o errno.o port.o
@


1.23
log
@Add errno.c to libc
@
text
@d12 1
a12 1
	pipe.o fdnull.o errno.o
@


1.22
log
@Add __ASM__ for #include crockery
@
text
@d12 1
a12 1
	pipe.o fdnull.o
@


1.21
log
@Source reorg
@
text
@d50 1
a50 1
	$(CPP) $(INCS) mach/syscalls.s tmp.s
@


1.20
log
@Add fdnull
@
text
@d4 1
a4 1
INCS= -I../include -I..
@


1.19
log
@Use del, not rm
@
text
@d12 1
a12 1
	pipe.o
@


1.18
log
@Fix make clean
@
text
@d36 1
a36 1
	$(CPP) mach/crt0.s > tmp.s
d38 1
a38 1
	rm -f tmp.s
d40 1
a40 1
	$(CPP) -DSRV mach/crt0.s > tmp.s
d42 1
a42 1
	rm -f tmp.s
d50 1
a50 1
	$(CPP) $(INCS) mach/syscalls.s  > tmp.s
d59 2
a60 1
	rm -f *.o *.a
@


1.17
log
@New files for boot-server specific versions of .o's
@
text
@d57 3
@


1.16
log
@Add pipe.c
@
text
@d6 2
a7 1
OBJS= crt0.o start.o syscalls.o syscall.o mem.o io.o setjmp.o malloc.o \
d17 2
d23 4
d34 1
d36 7
a42 1
	$(AS) -o crt0.o mach/crt0.s
@


1.15
log
@Add grp.c and pwd.c
@
text
@d10 2
a11 1
	perm.o random.o namer.o dir.o wait.o mktemp.o grp.o pwd.o
@


1.14
log
@Add mktemp.c
@
text
@d10 1
a10 1
	perm.o random.o namer.o dir.o wait.o mktemp.o
@


1.13
log
@Add wait.c
@
text
@d10 1
a10 1
	perm.o random.o namer.o dir.o wait.o
@


1.12
log
@Add dir.c directory functions
@
text
@d10 1
a10 1
	perm.o random.o namer.o dir.o
@


1.11
log
@Add random # generator, and move namer stuff into libc
@
text
@d10 1
a10 1
	perm.o random.o namer.o
@


1.10
log
@Start using optimizer
@
text
@d10 1
a10 1
	perm.o
@


1.9
log
@Add permission handling (umask/chmod/etc)
@
text
@d5 1
a5 1
CFLAGS= $(INCS) -g -DDEBUG
@


1.8
log
@Add fdmem
@
text
@d9 2
a10 1
	shell.o gnufuncs.o qsort.o float.o signal.o env.o fdmem.o
@


1.7
log
@Add env.o
@
text
@d9 1
a9 1
	shell.o gnufuncs.o qsort.o float.o signal.o env.o
@


1.6
log
@Add new files to get gcc up
@
text
@d9 1
a9 1
	shell.o gnufuncs.o qsort.o float.o signal.o
@


1.5
log
@Add shell.o
@
text
@d8 2
a9 1
	mount.o open.o stat.o fdl.o hash.o time.o exec.o getopt.o shell.o
d27 2
@


1.4
log
@Add getopt()
@
text
@d8 1
a8 1
	mount.o open.o stat.o fdl.o hash.o time.o exec.o getopt.o
@


1.3
log
@Add exec.o
@
text
@d8 1
a8 1
	mount.o open.o stat.o fdl.o hash.o time.o exec.o
@


1.2
log
@Add time.c
@
text
@d8 1
a8 1
	mount.o open.o stat.o fdl.o hash.o time.o
d26 2
@


1.1
log
@Initial revision
@
text
@d8 1
a8 1
	mount.o open.o stat.o fdl.o hash.o
@
