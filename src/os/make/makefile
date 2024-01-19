CC= gcc
AS= as
CPP= cpp
LD= ld
INCS= -I../../include -I.
DEFS= -DKERNEL -DDEBUG -DKDB
CFLAGS= $(DEFS) $(INCS) -O
LIBS= ../../libc/libusr.a

all: vsta

config: config.c
	$(CC) -o config config.c

locore.o: assym.h

assym.h: genassym
	./genassym assym.h

genassym: ../mach/genassym.c
	$(CC) $(INCS) -o genassym ../mach/genassym.c

locore.o: ../mach/locore.s
	$(CPP) $(INCS) $(DEFS) ../mach/locore.s locore.s
	$(AS) -o locore.o locore.s

init.o: ../mach/init.c
	$(CC) $(CFLAGS) -c ../mach/init.c

mutex.o: ../mach/mutex.c
	$(CC) $(CFLAGS) -c ../mach/mutex.c

vm.o: ../mach/vm.c
	$(CC) $(CFLAGS) -c ../mach/vm.c

hat.o: ../mach/hat.c
	$(CC) $(CFLAGS) -c ../mach/hat.c

dbg.o: ../mach/dbg.c
	$(CC) $(CFLAGS) -c ../mach/dbg.c

isr.o: ../mach/isr.c
	$(CC) $(CFLAGS) -c ../mach/isr.c

trap.o: ../mach/trap.c
	$(CC) $(CFLAGS) -c ../mach/trap.c

machproc.o: ../mach/machproc.c
	$(CC) $(CFLAGS) -c ../mach/machproc.c

syscall.o: ../mach/syscall.c
	$(CC) $(CFLAGS) -c ../mach/syscall.c

dbg_ibm.o: ../mach/dbg_ibm.c
	$(CC) $(CFLAGS) -c ../mach/dbg_ibm.c

dbg_ser.o: ../mach/dbg_ser.c
	$(CC) $(CFLAGS) -c ../mach/dbg_ser.c

main.o: ../kern/main.c
	$(CC) $(CFLAGS) -c ../kern/main.c

vm_fault.o: ../kern/vm_fault.c
	$(CC) $(CFLAGS) -c ../kern/vm_fault.c

vm_steal.o: ../kern/vm_steal.c
	$(CC) $(CFLAGS) -c ../kern/vm_steal.c

vm_page.o: ../kern/vm_page.c
	$(CC) $(CFLAGS) -c ../kern/vm_page.c

malloc.o: ../kern/malloc.c
	$(CC) $(CFLAGS) -c ../kern/malloc.c

misc.o: ../kern/misc.c
	$(CC) $(CFLAGS) -c ../kern/misc.c

vas.o: ../kern/vas.c
	$(CC) $(CFLAGS) -c ../kern/vas.c

pset.o: ../kern/pset.c
	$(CC) $(CFLAGS) -c ../kern/pset.c

msg.o: ../kern/msg.c
	$(CC) $(CFLAGS) -c ../kern/msg.c

msgcon.o: ../kern/msgcon.c
	$(CC) $(CFLAGS) -c ../kern/msgcon.c

msgkern.o: ../kern/msgkern.c
	$(CC) $(CFLAGS) -c ../kern/msgkern.c

seg.o: ../kern/seg.c
	$(CC) $(CFLAGS) -c ../kern/seg.c

port.o: ../kern/port.c
	$(CC) $(CFLAGS) -c ../kern/port.c

atl.o: ../kern/atl.c
	$(CC) $(CFLAGS) -c ../kern/atl.c

qio.o: ../kern/qio.c
	$(CC) $(CFLAGS) -c ../kern/qio.c

pset_fod.o: ../kern/pset_fod.c
	$(CC) $(CFLAGS) -c ../kern/pset_fod.c

pset_zfo.o: ../kern/pset_zfo.c
	$(CC) $(CFLAGS) -c ../kern/pset_zfo.c

pset_mem.o: ../kern/pset_mem.c
	$(CC) $(CFLAGS) -c ../kern/pset_mem.c

pset_cow.o: ../kern/pset_cow.c
	$(CC) $(CFLAGS) -c ../kern/pset_cow.c

vm_swap.o: ../kern/vm_swap.c
	$(CC) $(CFLAGS) -c ../kern/vm_swap.c

sched.o: ../kern/sched.c
	$(CC) $(CFLAGS) -c ../kern/sched.c

rand.o: ../kern/rand.c
	$(CC) $(CFLAGS) -c ../kern/rand.c

proc.o: ../kern/proc.c
	$(CC) $(CFLAGS) -c ../kern/proc.c

pview.o: ../kern/pview.c
	$(CC) $(CFLAGS) -c ../kern/pview.c

xclock.o: ../kern/xclock.c
	$(CC) $(CFLAGS) -c ../kern/xclock.c

event.o: ../kern/event.c
	$(CC) $(CFLAGS) -c ../kern/event.c

mmap.o: ../kern/mmap.c
	$(CC) $(CFLAGS) -c ../kern/mmap.c

phys.o: ../kern/phys.c
	$(CC) $(CFLAGS) -c ../kern/phys.c

exec.o: ../kern/exec.c
	$(CC) $(CFLAGS) -c ../kern/exec.c

exitgrp.o: ../kern/exitgrp.c
	$(CC) $(CFLAGS) -c ../kern/exitgrp.c

ptrace.o: ../kern/ptrace.c
	$(CC) $(CFLAGS) -c ../kern/ptrace.c

pstat.o: ../kern/pstat.c
	$(CC) $(CFLAGS) -c ../kern/pstat.c

dbgmain.o: ../dbg/dbgmain.c
	$(CC) $(CFLAGS) -c ../dbg/dbgmain.c

disasm.o: ../dbg/disasm.c
	$(CC) $(CFLAGS) -c ../dbg/disasm.c

dump.o: ../dbg/dump.c
	$(CC) $(CFLAGS) -c ../dbg/dump.c

expr.o: ../dbg/expr.c
	$(CC) $(CFLAGS) -c ../dbg/expr.c

lex.o: ../dbg/lex.c
	$(CC) $(CFLAGS) -c ../dbg/lex.c

mem.o: ../dbg/mem.c
	$(CC) $(CFLAGS) -c ../dbg/mem.c

names.o: ../dbg/names.c
	$(CC) $(CFLAGS) -c ../dbg/names.c

dbgproc.o: ../dbg/dbgproc.c
	$(CC) $(CFLAGS) -c ../dbg/dbgproc.c

trace.o: ../dbg/trace.c
	$(CC) $(CFLAGS) -c ../dbg/trace.c

dbgio.o: ../dbg/dbgio.c
	$(CC) $(CFLAGS) -c ../dbg/dbgio.c

OBJS=locore.o init.o mutex.o vm.o hat.o dbg.o isr.o \
	trap.o machproc.o syscall.o dbg_ibm.o dbg_ser.o \
	main.o vm_fault.o vm_steal.o vm_page.o malloc.o \
	misc.o vas.o pset.o msg.o msgcon.o msgkern.o \
	seg.o port.o atl.o qio.o pset_fod.o pset_zfo.o \
	pset_mem.o pset_cow.o vm_swap.o sched.o rand.o \
	proc.o pview.o xclock.o event.o mmap.o phys.o \
	exec.o exitgrp.o ptrace.o pstat.o dbgmain.o \
	disasm.o dump.o expr.o lex.o mem.o names.o \
	dbgproc.o trace.o dbgio.o

vsta: $(OBJS) dbsym
	$(LD) -e _start -o vsta $(OBJS) $(LIBS)
	./dbsym vsta

dbsym: ../dbg/dbsym.c
	$(CC) $(DEFS) -o dbsym ../dbg/dbsym.c

clean:
	rm -f genassym assym.h locore.s *.o dbsym config

clobber: clean
	rm -f vsta

install: vsta
	strip vsta
	cp vsta ../../boot
