CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

etap4: etap4.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o etap4 etap4.c $(LDLIBS)

etap3: etap3.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o etap3 etap3.c $(LDLIBS)

etap2: etap2.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o etap2 etap2.c $(LDLIBS)

etap1: etap1.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o etap1 etap1.c $(LDLIBS)

all: etap1 etap2 etap3 etap4

.PHONY: clean all

clean:
	rm -f etap1 etap2 etap3 etap4
