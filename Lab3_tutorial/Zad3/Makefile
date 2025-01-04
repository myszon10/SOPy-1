CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

etap2: etap2.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o etap2 etap2.c $(LDLIBS)

etap1: etap1.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o etap1 etap1.c $(LDLIBS)

all: etap1 etap2

.PHONY: clean all

clean:
	rm -f etap1 etap2
