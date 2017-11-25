CC = gcc
CFLAGS = -g -c
CFLAGS2 = -g -w
AR = ar -rc
RANLIB = ranlib

Target: my_memory.a

all:: memoryTest

memoryTest:
	gcc -g -w -o memoryTest memoryTest.c -L./ -lmy_memory

my_memory.a: my_memory.o
	$(AR) libmy_memory.a my_memory.o
	$(RANLIB) libmy_memory.a

my_memory.o: my_memory.h
	$(CC) $(CFLAGS) my_memory.c

clean:
	rm -rf testfile *.o *.a
