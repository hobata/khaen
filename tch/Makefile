# Makefile for touch

CC            = gcc
#CFLAGS        = -Wall -O3 -mtune=native -march=native
CFLAGS        = -Wall -mtune=native -march=native
LIBS          = -lwiringPi
OBJS          = tch_cntl.o touch.o
PROGRAM       = touch

all:            $(PROGRAM)

$(PROGRAM):	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(PROGRAM)

clean:;         rm -f *.o *~ $(PROGRAM)
release:;       rm -f *.o *~
