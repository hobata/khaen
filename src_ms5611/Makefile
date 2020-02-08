# Makefile for khaen
# sudo apt-get install liasound2-dev

CC            = gcc
CFLAGS        = -Wall -Iinclude -O3 -mtune=native -march=native
LIBS          = -lrt -lm -lasound -lwiringPi -lconfig
OBJS          = midi.o conf.o cmp.o rec.o btn.o key.o led.o main.o pcm.o common.o khaen.o log.o monitor.o press.o
PROGRAM       = khaen

all:            $(PROGRAM)

$(PROGRAM):	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(PROGRAM)

clean:;         rm -f *.o *~ $(PROGRAM)
release:;       rm -f *.o *~
