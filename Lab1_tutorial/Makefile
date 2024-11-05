CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: prog1

prog1:
	gcc $(CFLAGS) -o prog1 prog1.c

.PHONY: clean all

clean:
	rm prog1