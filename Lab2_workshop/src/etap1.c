#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void usage(int argc, char* argv[])
{
    printf("%s n f \n", argv[0]);
    printf("\tf - file to be processed\n");
    printf("\t0 < n < 10 - number of child processes\n");
    exit(EXIT_FAILURE);
}

void child_work() {
    pid_t pid = getpid();
    printf("[%d] hello im child with pid %d\n", pid, pid);
}

int main(int argc, char* argv[])
{
    if(argc != 3) {
        usage(argc, argv);
    }
    if('0' >= argv[2][0] || argv[2][0] > '9') {
        usage(argc, argv);
    }
    if(argv[2][1] != '\0') {
        usage(argc, argv);
    }

    for(char i = '0'; i < argv[2][0]; i++) {
        switch(fork()) {
        case 0:
            child_work();
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
        default:
            break;
        }
    }
    while(wait(NULL) > 0);
    return EXIT_SUCCESS;
}
