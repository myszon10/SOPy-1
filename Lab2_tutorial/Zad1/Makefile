CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

etap2:
	gcc $(CFLAGS) -o etap2 etap2.c

etap1:
	gcc $(CFLAGS) -o etap1 etap1.c

.PHONY: clean all etap1 etap2

all: etap1 etap2

clean:
	rm etap1 etap2
