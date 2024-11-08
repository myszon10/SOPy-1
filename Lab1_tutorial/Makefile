CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: prog1 prog2 prog3 prog4 progEks prog5 quiz1

prog1:
	gcc $(CFLAGS) -o prog1 prog1.c

prog2:
	gcc $(CFLAGS) -o prog2 prog2.c

prog3:
	gcc $(CFLAGS) -o prog3 prog3.c

prog4:
	gcc $(CFLAGS) -o prog4 prog4.c

progEks:
	gcc $(CFLAGS) -o progEks progEks.c

prog5:
	gcc $(CFLAGS) -o prog5 prog5.c 

quiz1:
	gcc $(CFLAGS) -o quiz1 quiz1.c

.PHONY: clean all

clean:
	rm prog1 prog2 prog3 prog4 progEks prog5 quiz1