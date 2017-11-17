CC = gcc
CFLAGS = -g -w -std=gnu99

all:: parallelCal vectorMultiply externalCal

parallelCal: 
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c -L../ -lmy_pthread -L../MyMemory -lmy_memory

vectorMultiply: 
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c -L../ -lmy_pthread -L../MyMemory -lmy_memory

externalCal: 
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c -L../ -lmy_pthread -L../MyMemory -lmy_memory

clean:
	rm -rf parallelCal vectorMultiply externalCal *.o
