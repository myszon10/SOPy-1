CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: etap1 etap2

etap1:
	gcc $(CFLAGS) -o etap1 etap1.c 

etap2:
	gcc $(CFLAGS) -o etap2 etap2.c

.PHONY: clean all

clean:
	rm etap1 etap2