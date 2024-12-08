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

//void SLEEP() {
//    struct timespec t = {0, 250 * 1000 * 1000};
//    nanosleep(t, NULL);
//}

void child_sigusr1_handler(int sig) {
    // noop
}

void child_work(char *buf, ssize_t buf_size) {
    sigset_t mask;
    sigemptyset(&mask);
    sigsuspend(&mask); // Czeka na dowolny sygnał - tu SIGUSR1

    bulk_write(1, buf, buf_size); // 1 to fd stdout
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
    int n = atoi(argv[2]);

    int fd;
    if((fd = open(argv[1], O_RDONLY)) == -1) {
        ERR(argv[1]);
    }
    int file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); // wraca kursor na początek pliku


    char *buf;
    ssize_t child_buf_size;
    size_t buf_size = file_size / n + 1;
    buf = malloc(buf_size); // bez '\0'
    if(!buf) {
        ERR("malloc");
    }
    pid_t children[n];

    sethandler(child_sigusr1_handler, SIGUSR1);

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    pid_t pid;
    for(int i = 0; i < n; i++) {
        if((child_buf_size = bulk_read(fd, buf, buf_size)) < 0) {
            ERR("bulk read");
        }
        pid = fork();
        switch(pid) {
        case 0:
            child_work(buf, child_buf_size);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
        default:
            children[i] = pid;
            break;
        }
    }
    free(buf);

    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    sethandler(SIG_IGN, SIGUSR1);
    kill(0, SIGUSR1);

    while(wait(NULL) > 0);
    return EXIT_SUCCESS;
}
