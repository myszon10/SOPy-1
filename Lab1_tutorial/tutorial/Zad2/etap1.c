#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
    ({ long int __result;               \
       do __result = (long int)(expression); \
       while (__result == -1L && errno == EINTR); \
       __result; })
#endif

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

int sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        return -1;
    return 0;
}

// Obsługa sygnału SIGCHLD - zapobiega powstawaniu zombie procesów
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
            ERR("waitpid:"); // Obsługa błędu
        }
    }
}

// Funkcja wykonywana przez proces potomny
void child_work(int fd, int R)
{
    srand(getpid());  
    char c = 'a' + rand() % ('z' - 'a');  // Losowanie litery z przedziału [a-z]
    
    if (write(R, &c, 1) < 0)  // Wysyłanie litery do potoku R
        ERR("write to R");
}

// Funkcja wykonywana przez proces rodzica - odbieranie danych od dzieci
void parent_work(int n, int *fds, int R)
{
    char c;
    int status;
    srand(getpid());
    
    while ((status = TEMP_FAILURE_RETRY(read(R, &c, 1))) == 1)  // Odczytywanie danych znak po znakuS
        printf("%c", c);
    
    if (status < 0)  // Sprawdzenie błędu odczytu
        ERR("read from R");
    
    printf("\n"); // Po zakończeniu odczytu dodajemy nową linię
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

    if (NULL == (fds = (int *)malloc(sizeof(int) * n)))  // Alokacja tablicy deskryptorów
        ERR("malloc");

    if (sethandler(sigchld_handler, SIGCHLD)) 
        ERR("Setting parent SIGCHLD:");

    create_children_and_pipes(n, fds, R[1]);

    if (close(R[1]))  // Rodzic zamyka koniec do pisania w R
        ERR("close");

    parent_work(n, fds, R[0]);

    while (n--)  // Zamykanie deskryptorów potoków P1, P2, ..., Pn
        if (fds[n] && close(fds[n]))
            ERR("close");

    if (close(R[0]))  // Zamknięcie potoku R[0] po odczycie
        ERR("close");

    free(fds); 
    return EXIT_SUCCESS;
}