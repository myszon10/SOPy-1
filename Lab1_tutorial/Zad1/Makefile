CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: etap12 etap34

etap12:
	gcc $(CFLAGS) -o etap12 etap12.c 

etap34:
	gcc $(CFLAGS) -o etap34 etap34.c

.PHONY: clean all

clean:
	rm etap12 etap34