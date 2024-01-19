CC=gcc
LD=ld
CFLAGS=-I../../include -O

OBJS = check.o input.o macro.o main.o make.o reader.o rules.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

make: $(OBJS)
	$(LD) -o make ../../libc/crt0.o $(OBJS) -L../../libc -lc
