# Makefile for khaen
# sudo apt-get install liasound2-dev

CC            = gcc
#CFLAGS        = -Wall -g -Iinclude -O3 -mtune=native -march=native
CFLAGS        = -Wall -march=armv6 -mfpu=vfp -mfloat-abi=hard -g -Iinclude -O3 -mtune=native
LIBS          = -lm -lasound -lwiringPi
OBJS          = rec.o btn.o key.o led.o main.o pcm.o common.o khaen.o log.o monitor.o press.o compress.o
PROGRAM       = khaen

all:            $(PROGRAM)

$(PROGRAM):	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(PROGRAM)

clean:;         rm -f *.o *~ $(PROGRAM)
release:;       rm -f *.o *~
