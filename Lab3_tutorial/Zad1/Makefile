CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

all: etap1

etap1: etap1.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o etap1 etap1.c $(LDLIBS)

.PHONY: clean all

clean:
	rm -f etap1