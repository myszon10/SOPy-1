CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined

all: prog13a prog13b prog14 prog15 prog16a

prog13a:
	gcc $(CFLAGS) -o prog13a prog13a.c

prog13b:
	gcc $(CFLAGS) -o prog13b prog13b.c

prog14:
	gcc $(CFLAGS) -o prog14 prog14.c

prog15:
	gcc $(CFLAGS) -o prog15 prog15.c

prog16a:
	gcc $(CFLAGS) -o prog16a prog16a.c

.PHONY: clean all

clean:
	rm prog13a prog13b prog14 prog15 prog16a