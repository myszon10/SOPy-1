#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "3<n<=20 - number of students\n");
    exit(EXIT_FAILURE);
}

void child_work() {
    printf("Child with pid: %d\n", getpid());
}

int main(int argc, char **argv) {
    if(argc != 2) {
        usage(argv[0]);
    }

    int n = atoi(argv[1]);
    if(n < 3 || n > 20) {
        usage(argv[0]);
    }

    for(int i = 0; i < n; i++) {
        switch(fork()) {
            case 0:
                child_work();
                exit(EXIT_SUCCESS);
                break;   
        }
    }
}