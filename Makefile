CC = g++
CFLAGS = -Wall -O2

mountain: mountain.cpp fcyc2.c clock.c
 $(CC) $(CFLAGS) -o mountain mountain.cpp fcyc2.c clock.c

clean:
 rm -f mountain *.o *~