CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

all: prog17 prog18

prog17: prog17.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o prog17 prog17.c $(LDLIBS)

prog18: prog18.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o prog18 prog18.c $(LDLIBS)

.PHONY: clean all

clean:
	rm -f prog17 prog18
