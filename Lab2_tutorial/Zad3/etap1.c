#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))

#define BUFFER_SIZE 256

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s P T1 T2\n", name);
    exit(EXIT_FAILURE);
}

void child_work(pid_t prev_pid, char *buf)
{
    printf("[%d] %s has joined the game! (after %d)\n", getpid(), buf, prev_pid);
}

int main(int argc, char **argv)
{
    if(argc != 4) 
        usage(argv[0]);

    int P = atoi(argv[1]);
    int T1 = atoi(argv[2]);
    int T2 = atoi(argv[3]);

    char buf[BUFFER_SIZE];

    pid_t res;
    pid_t prev_pid = getpid();

    while (1) {
        if (!fgets(buf, sizeof(buf), stdin))
            ERR("fgets");

        buf[strlen(buf) - 1] = '\0';

        if (strncmp(buf, "start", 5)) {
            switch ((res = fork())) {
            case -1:
                ERR("fork");
            case 0:
                child_work(prev_pid, buf);
                
                exit(EXIT_SUCCESS);
            default:
                prev_pid = res;
                break;
            }
        }
        else break;
    }
}