CC=gcc
CFLAGS=-std=gnu99 -Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
LDLIBS=-lpthread -lm

SRC1=etap1.c circular_buffer.c
SRC2=etap2.c circular_buffer.c
SRC3=etap3.c circular_buffer.c
SRC4=etap4.c circular_buffer.c



OBJ1=$(SRC1:.c=.o)
OBJ2=$(SRC2:.c=.o)
OBJ3=$(SRC3:.c=.o)
OBJ4=$(SRC4:.c=.o)


all: etap4

etap1: $(OBJ1)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

etap2: $(OBJ2)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

etap3: $(OBJ3)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

etap4: $(OBJ4)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean all

clean:
	rm -f $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) etap1 etap2 etap3 etap4
