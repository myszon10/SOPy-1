override CFLAGS=-Wall -Wextra -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Werror
endif

.PHONY: clean all

all: sop-grep

sop-grep: sop-grep.c	
	gcc $(CFLAGS) -o sop-grep sop-grep.c

clean:
	rm sop-grep
