#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

// MAX_BUFF must be in one byte range
#define MAX_BUFF 200

volatile sig_atomic_t last_signal = 0;

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

void sig_handler(int sig) { last_signal = sig; }

void sig_killme(int sig)
{
    if (rand() % 5 == 0)
        exit(EXIT_SUCCESS);
}

// Obsługa SIGCHLD - zapobiega zombie procesom
void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG); // Sprawdzamy, czy jakieś dziecko się zakończyło
        if (0 == pid)
            return; // Jeśli nie ma zakończonych procesów, kończymy obsługę
        if (0 >= pid)
        {
            if (ECHILD == errno)
                return; // Jeśli nie ma już żadnych dzieci, kończymy obsługę
            ERR("waitpid:");
        }
    }
}

void child_work(int fd, int R)
{
    char c, buf[MAX_BUFF + 1];
    unsigned char s;
    srand(getpid());
    if (sethandler(sig_killme, SIGINT)) // Dziecko obsługuje SIGINT (może zakończyć działanie)
        ERR("Setting SIGINT handler in child");
    for (;;)
    {
        if (TEMP_FAILURE_RETRY(read(fd, &c, 1)) < 1) // Oczekiwanie na znak od rodzica
            ERR("read");
        s = 1 + rand() % MAX_BUFF;
        buf[0] = s;
        memset(buf + 1, c, s); // Wypełnienie bufora znakiem `c`
        if (TEMP_FAILURE_RETRY(write(R, buf, s + 1)) < 0) // Wysłanie danych do rodzica
            ERR("write to R"); 
    }
}

// Funkcja obsługująca rodzica - odbiór danych od dzieci, reagowanie na SIGINT
void parent_work(int n, int *fds, int R)
{
    unsigned char c;
    char buf[MAX_BUFF];
    int status, i;
    srand(getpid());
    if (sethandler(sig_handler, SIGINT)) // Rodzic obsługuje SIGINT
        ERR("Setting SIGINT handler in parent");
    for (;;)
    {
        if (SIGINT == last_signal)
        {
            i = rand() % n; // Losowanie dziecka
            while (0 == fds[i % n] && i < 2 * n) // Jeśli dziecko już nie działa, szukamy kolejnego
                i++;
            i %= n;
            if (fds[i])
            {
                c = 'a' + rand() % ('z' - 'a');
                status = TEMP_FAILURE_RETRY(write(fds[i], &c, 1)); // Wysłanie znaku do dziecka
                if (status != 1)
                {
                    if (TEMP_FAILURE_RETRY(close(fds[i]))) // Zamknięcie deskryptora, jeśli dziecko nie odpowiada
                        ERR("close");
                    fds[i] = 0;
                }
            }
            last_signal = 0;
        }
        status = read(R, &c, 1); // Odczyt pierwszego bajtu (rozmiaru bufora)
        if (status < 0 && errno == EINTR)
            continue;
        if (status < 0)
            ERR("read header from R");
        if (0 == status)
            break;
        if (TEMP_FAILURE_RETRY(read(R, buf, c)) < c) // Odczyt całego bufora od dziecka
            ERR("read data from R");
        buf[(int)c] = 0;
        printf("\n%s\n", buf);  // Wypisanie otrzymanego bufora
    }
}

// Tworzenie procesów potomnych i ich potoków
void create_children_and_pipes(int n, int *fds, int R)
{
    int tmpfd[2];  // Tymczasowy pipe dla każdego dziecka
    int max = n;   // Zapamiętujemy początkową liczbę dzieci
    
    while (n)
    {
        if (pipe(tmpfd))  // Tworzymy nowy pipe dla dziecka
            ERR("pipe");

        switch (fork()) 
        {
            case 0:  // Kod wykonywany przez dziecko
                while (n < max)  // Zamykamy nieużywane deskryptory w dziecku
                    if (fds[n] && close(fds[n++]))
                        ERR("close");
                
                free(fds);  // Zwolnienie niepotrzebnie skopiowanej tablicy rodzic->dziecko
                if (close(tmpfd[1]))  // Zamykamy koniec do pisania w dziecku
                    ERR("close");
                
                child_work(tmpfd[0], R);
                
                if (close(tmpfd[0]))  // Zamykamy koniec do czytania
                    ERR("close");
                if (close(R))  // Zamykamy potok R w dziecku (żeby mógł się zamknąć u rodzica)
                    ERR("close");
                
                exit(EXIT_SUCCESS);  // Kończymy proces potomny

            case -1:
                ERR("Fork:");
        }

        if (close(tmpfd[0]))  // Rodzic zamyka koniec do czytania, bo będzie pisał
            ERR("close");
        
        fds[--n] = tmpfd[1];  // Zapisujemy deskryptor do tablicy fds
    }
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "0<n<=10 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int n, *fds, R[2];

    if (2 != argc)
        usage(argv[0]);

    n = atoi(argv[1]); 
    if (n <= 0 || n > 10)  
        usage(argv[0]);

    if (pipe(R))  // Tworzymy potok R do komunikacji dziecko -> rodzic
        ERR("pipe");

    if (NULL == (fds = (int *)malloc(sizeof(int) * n)))  
        ERR("malloc");

    if (sethandler(sigchld_handler, SIGCHLD))  // Ustawienie obsługi SIGCHLD, aby unikać zombie procesów
        ERR("Setting parent SIGCHLD:");

    create_children_and_pipes(n, fds, R[1]); 

    if (close(R[1]))  // Rodzic zamyka koniec do pisania w R, ponieważ tylko dzieci będą pisać do tego potoku
        ERR("close");

    parent_work(n, fds, R[0]);  // Rodzic odbiera i wypisuje dane z potoku R

    while (n--)  // Zamykanie deskryptorów potoków P1, P2, ..., Pn
        if (fds[n] && close(fds[n]))
            ERR("close");

    if (close(R[0]))  // Zamknięcie potoku R[0] po odczycie
        ERR("close");

    free(fds); 
    return EXIT_SUCCESS;
}
