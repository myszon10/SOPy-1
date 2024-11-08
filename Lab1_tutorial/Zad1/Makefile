CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: etap12

etap12:
	gcc $(CFLAGS) -o etap12 etap12.c 

.PHONY: clean all

clean:
	rm etap12