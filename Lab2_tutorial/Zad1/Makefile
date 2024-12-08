CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

etap3:
	gcc $(CFLAGS) -o etap3 etap3.c

etap2:
	gcc $(CFLAGS) -o etap2 etap2.c

etap1:
	gcc $(CFLAGS) -o etap1 etap1.c

.PHONY: clean all etap1 etap2 etap3

all: etap1 etap2 etap3

clean:
	rm etap1 etap2 etap3 *.txt
