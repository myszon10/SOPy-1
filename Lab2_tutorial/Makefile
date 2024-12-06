CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: prog13a prog13b

prog13a:
	gcc $(CFLAGS) -o prog13a prog13a.c

prog13b:
	gcc $(CFLAGS) -o prog13b prog13b.c

.PHONY: clean all

clean:
	rm prog13a prog13b