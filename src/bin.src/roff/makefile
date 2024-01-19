CC=gcc
LD=ld
CFLAGS=-O -I../../include
OBJS= roff.o roff1.o itoeng.o
CRT0=../../libc/crt0.o
LIBS=-L../../libc -lc

.c.o:
	$(CC) $(CFLAGS) -c $*.c

roff: $(OBJS)
	$(LD) -o roff $(CRT0) $(OBJS) $(LIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f roff

install: roff
	strip roff
	cp roff ../../bin
