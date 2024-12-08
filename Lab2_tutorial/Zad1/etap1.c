#define _GNU_SOURCE
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

ssize_t bulk_write(int fd, char *buf, size_t count)
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

void usage(char* progname) {
    fprintf(stderr, "usage for '%s'\n", progname);
    kill(0, SIGKILL);
    exit(EXIT_FAILURE);
}

void child_work(char n) {
    srand(time(NULL) * getpid());
    int s = (10 + rand() % (100 - 10 + 1));

    printf("[%d] n: %c s: %d\n", getpid(), n, s);
}

void parent_work() {
    struct timespec t = {0, 10 * 1000 * 1000};
    for(int i = 0; i < 100; i++) {
        nanosleep(&t, NULL);
        if(kill(0, SIGUSR1)) {
            ERR("kill");
        }
    }
    while (wait(NULL) > 0);
}

int main(int argc, char **argv) {
    for(int i = 1; i < argc; i++) {
        if('0' <= argv[i][0] && argv[i][0] <= '9') {
            if(argv[i][1] != '\0') {
                usage(argv[0]);
            }
            switch(fork()) {
                case 0:
                    child_work(argv[i][0]);
                    exit(EXIT_SUCCESS);
                case -1:
                    ERR("fork");
                default:
                    break;
            }
        } else {
            usage(argv[0]);
        }
    }

    sethandler(SIG_IGN, SIGUSR1);

    parent_work();
    return EXIT_SUCCESS;
}
