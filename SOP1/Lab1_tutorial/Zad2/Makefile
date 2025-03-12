CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: etap1 etap2 etap3 etap4

etap1:
	gcc $(CFLAGS) -o etap1 etap1.c 

etap2:
	gcc $(CFLAGS) -o etap2 etap2.c

etap3:
	gcc $(CFLAGS) -o etap3 etap3.c

etap4:
	gcc $(CFLAGS) -o etap4 etap4.c

.PHONY: clean all

clean:
	rm etap1 etap2 etap3 etap4