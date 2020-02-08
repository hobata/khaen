# Makefile for khaen

CC            = gcc
CFLAGS        = -MMD -Wall -MP -O3 -mtune=native -march=native
LDFLAGS       = -lrt -lm -lasound -lwiringPi -lconfig
LIBS	      =
INCLUDE = -I ./include
SRC_DIR = ./c_src
OBJ_DIR = ./build
SOURCES = $(shell ls $(SRC_DIR)/*.c) 
OBJS    = $(subst $(SRC_DIR),$(OBJ_DIR), $(SOURCES:.c=.o))
TARGET       = khaen
DEPENDS = $(OBJS:.o=.d)

all:            $(TARGET)

$(TARGET):	$(OBJS) $(LIBS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
		$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $< 

clean:;         rm $(OBJS) $(TARGET) $(DEPENDS)
