#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t sigusr2_count = 0; // Licznik sygnałów SIGUSR2

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sig_handler(int sig) 
{ 
    last_signal = sig;
    if (sig == SIGUSR2)
        sigusr2_count++; 
}

void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
    }
}

void child_work(int m, int p) // p - co ile sygnałów SIGUSR1 wysyłamy sygnał SIGUSR2
{
    int count = 0;
    struct timespec t = {0, m * 10000};
    while (1)
    {
        for (int i = 0; i < p; i++)
        {
            nanosleep(&t, NULL);
            if (kill(getppid(), SIGUSR1)) // getppid - wysyłanie sygnału do rodzica
                ERR("kill");
        }
        nanosleep(&t, NULL);
        if (kill(getppid(), SIGUSR2))
            ERR("kill");
        count++;
        printf("[%d] sent %d SIGUSR2\n", getpid(), count);
    }
}

void parent_work(sigset_t oldmask)
{
    int printed_count = 0;
    while (1)
    {
        last_signal = 0;
        sigsuspend(&oldmask);
        // sigsuspend wstrzymuje działanie kodu do momentu odebrania sygnału
        // tymczasowo odblokowuje SIGUSR1 i SIGUSR2 (nałożoną maskę)
        if (printed_count != sigusr2_count) // lub while(last_signal != SIGUSR2)
        {
            printf("[PARENT] received %d SIGUSR2\n", printed_count);
            printed_count = sigusr2_count;
        }
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s m  p\n", name);
    fprintf(stderr,
            "m - number of 1/1000 milliseconds between signals [1,999], "
            "i.e. one milisecond maximum\n");
    fprintf(stderr, "p - after p SIGUSR1 send one SIGUSER2  [1,999]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int m, p;
    if (argc != 3)
        usage(argv[0]);
    m = atoi(argv[1]);
    p = atoi(argv[2]);
    if (m <= 0 || m > 999 || p <= 0 || p > 999)
        usage(argv[0]);
    sethandler(sigchld_handler, SIGCHLD);
    sethandler(sig_handler, SIGUSR1);
    sethandler(sig_handler, SIGUSR2);
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    // dodanie SIGUSR1/2 do maski zablokowanych sygnałów
    // sigprocmask umożliwia potem skorzystanie z sigsuspend
    sigprocmask(SIG_BLOCK, &mask, &oldmask); 
    pid_t pid;
    if ((pid = fork()) < 0)
        ERR("fork");
    if (0 == pid)
        child_work(m, p);
    else
    {
        parent_work(oldmask);
        while (wait(NULL) > 0)
            ;
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    return EXIT_SUCCESS;
}