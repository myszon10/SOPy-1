override CFLAGS=-Wall -Wextra -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Werror
endif

.PHONY: clean all

all: sop-vp

sop-vp: sop-vp.c video-player.h	
	gcc $(CFLAGS) -o sop-vp sop-vp.c

clean:
	rm -f sop-vp
