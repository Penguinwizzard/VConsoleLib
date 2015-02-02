CFLAGS= -c -std=c99 -Wall -g
LFLAGS= -g
CC=gcc

all: test dump
dump: dump.c
	$(CC) $(LFLAGS) dump.c -o dump
test: VConsoleLib.o test.o
	$(CC) $(LFLAGS) VConsoleLib.o test.o -o test
VConsoleLib.o: VConsoleLib.h VConsoleLib.c
	$(CC) VConsoleLib.c $(CFLAGS)
test.o: VConsoleLib.h test.c
	$(CC) test.c $(CFLAGS)
clean:
	rm *.o test dump
