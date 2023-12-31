CC = g++
CFLAGS = -Wall -O2

memory_mountain: memory_mountain.cpp fcyc2.c clock.c
    $(CC) $(CFLAGS) -o memory_mountain memory_mountain.cpp fcyc2.c clock.c -lpthread

clean:
    rm -f memory_mountain *.o *~