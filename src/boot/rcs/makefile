head	1.3;
access;
symbols
	V1_3_1:1.3
	V1_3:1.3
	V1_2:1.2
	V1_1:1.2
	V1_0:1.2;
locks; strict;
comment	@# @;


1.3
date	94.04.06.21.55.21;	author vandys;	state Exp;
branches;
next	1.2;

1.2
date	93.06.27.17.39.00;	author vandys;	state Exp;
branches;
next	1.1;

1.1
date	93.01.29.15.42.22;	author vandys;	state Exp;
branches;
next	;


desc
@Makefile
@


1.3
log
@Add clean, clobber, install targets
@
text
@OBJS= main.obj ptes.obj seg.obj load.obj gdt.obj move.obj util.obj
CC= bcc
AS= tasm
CFLAGS= -v -g1 -ml
ASFLAGS= -mx

.c.obj:
	$(CC) $(CFLAGS) -c $*.c
.asm.obj:
	$(AS) $(ASFLAGS) $*.asm

boot: $(OBJS)
	$(CC) $(CFLAGS) -eboot.exe $(OBJS)

clean:
	rm -f *.obj

clobber: clean
	rm -f boot

install:

@


1.2
log
@New file util.asm
@
text
@d16 7
a22 1
	rm -f *.obj boot
@


1.1
log
@Initial revision
@
text
@d1 1
a1 1
OBJS= main.obj ptes.obj seg.obj load.obj gdt.obj move.obj
a13 3

move.asm: move.s
	copy move.s move.asm
@
