CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: prog1 prog2

prog1:
	gcc $(CFLAGS) -o prog1 prog1.c

prog2:
	gcc $(CFLAGS) -o prog2 prog2.c

.PHONY: clean all

clean:
	rm prog1 prog2