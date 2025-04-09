CC=gcc
CFLAGS=-std=gnu99 -Wall

all: l7-2_server, l7-2_client

l7-1: l7-1.c
	$(CC) $(CFLAGS) -o l7-1 l7-1.c

l7-2_server: l7-2_server.c
	$(CC) $(CFLAGS) -o l7-2_server l7-2_server.c

l7-2_client: l7-2_client.c
	$(CC) $(CFLAGS) -o l7-2_client l7-2_client.c

clean:
	rm -f l7-1 l7-2_client l7-2_server