#define _GNU_SOURCE
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

void SLEEP() {
    struct timespec t = {0, 250 * 1000 * 1000};
    nanosleep(&t, NULL);
}

void parent_sigint_handler(int sig) {
    sethandler(SIG_IGN, SIGINT);
    kill(0, SIGINT);
    sethandler(SIG_DFL, SIGINT);
    //exit(EXIT_SUCCESS);
}

volatile sig_atomic_t got_sigint = 0;
void child_sigint_handler(int sig) {
    got_sigint = 1;
}

void child_sigusr1_handler(int sig) {
    // noop
}

void child_work(char *file, int nr, char *buf, ssize_t buf_size) {
    // sigset_t mask;
    // sigemptyset(&mask);
    // sigsuspend(&mask); // Czeka na dowolny sygnał - tu SIGUSR1

    // "Poprawniejszy sposób"
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGINT);
    sigsuspend(&mask);


    char filename[32];
    sprintf(filename, "%s-%d", file, nr);
    int fd;
    if((fd = TEMP_FAILURE_RETRY(open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666))) < 0) {
        ERR("open");
    }

    for(int i = 0; i < buf_size && !got_sigint; i++) {
        if(isalpha(buf[i]) && i % 2 == 0) {
            if(islower(buf[i])) {
                buf[i] = toupper(buf[i]);
            } else {
                buf[i] = tolower(buf[i]);
            }
        }
        SLEEP();
        if(TEMP_FAILURE_RETRY(write(fd, buf + i, 1)) <= 0) {
            ERR("write");
        }
    }

    if(TEMP_FAILURE_RETRY(close(fd))) {
        ERR("close");
    }
    free(buf);
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

    sethandler(child_sigusr1_handler, SIGUSR1);
    sethandler(child_sigint_handler, SIGINT);

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
            child_work(argv[1], i, buf, child_buf_size);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
        default:
            break;
        }
    }
    free(buf);

    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    sethandler(SIG_IGN, SIGUSR1);
    sethandler(parent_sigint_handler, SIGINT);

    kill(0, SIGUSR1);

    while(TEMP_FAILURE_RETRY(wait(NULL)) > 0);
    return EXIT_SUCCESS;
}
