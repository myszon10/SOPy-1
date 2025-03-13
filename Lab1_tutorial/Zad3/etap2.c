#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define BUF_SIZE 256

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n\n", name);
    fprintf(stderr, "3<n<=20 - number of students\n");
    exit(EXIT_FAILURE);
}

// outputPipe taki sam dla każdego dziecka, inputPipe indywidualny
void child_work(int inputPipe, int outputPipe) {
    char buffer[BUF_SIZE];

    // odczytanie komunikatu od nauczyciela
    ssize_t count = read(inputPipe, buffer, BUF_SIZE);
    if(count < 0) 
        ERR("read");

    // przygotowanie odpowiedzi
    pid_t pid = getpid();
    char response[BUF_SIZE];
    snprintf(response, BUF_SIZE, "Student %d: HERE", pid);
    
    // wysyłanie odpowiedzi
    printf("%s\n", response);
    if(write(outputPipe, response, BUF_SIZE) < 0)
        ERR("write");

    close(inputPipe);
    close(outputPipe);
    exit(EXIT_SUCCESS);
}

void parent_work(int n, int *outputPipes, int inputPipe, pid_t *pids) {
    char buffer[BUF_SIZE];
    
    for(int i = 0; i < n; i++) {
        snprintf(buffer, BUF_SIZE, "Teacher: Is %d here?\n", pids[i]);

        // Wypisanie do stdout
        printf("%s", buffer);
        
        // Wypisanie do indywidualnych strumieni dzieci
        if(write(outputPipes[i], buffer, BUF_SIZE) < 0) 
            ERR("write");

        // Słuchanie odpowiedzi od procesów dzieci
        if(read(inputPipe, buffer, BUF_SIZE) < 0)
            ERR("read");
    }
}

// outputPipes + input pipy dla każdego dziecka
void create_children_and_pipes(int n, int *outputPipes, int childOutputPipe, pid_t *pids) {
    int tmpfd[2]; // [0] - stdin, [1] - stdout

    for(int i = 0; i < n; i++) {
        if(pipe(tmpfd)) // jedna z wielu strzałek
            ERR("pipe");
        
        outputPipes[i] = tmpfd[1]; // część strzałki bez grota
        pid_t pid = fork();
        pids[i] = pid;

        switch(pid) {
            case 0:// kod dziecka
                for(int j = 0; j < i; j++)
                    if(close(outputPipes[j]))
                        ERR("close");
                
                free(outputPipes);
                free(pids);

                child_work(tmpfd[0], childOutputPipe); // Odpowiadanie dziecka

                if (close(tmpfd[0]))  // Zamykamy koniec do czytania
                    ERR("close");
                
                
                exit(EXIT_SUCCESS);
            case -1:
                ERR("fork");

        }

        if(close(tmpfd[0])) // zamykanie części z grotem małej strzałki
            ERR("close");

    }
}

int main(int argc, char **argv) {
    int n, *outputPipes, mainPipe[2];
    
    if(argc != 2) {
        usage(argv[0]);
    }

    n = atoi(argv[1]);
    if(n < 3 || n > 20) {
        usage(argv[0]);
    }

    if(pipe(mainPipe))
        ERR("pipe");

    if (NULL == (outputPipes = (int *)malloc(sizeof(int) * n))) 
        ERR("malloc");

    pid_t *pids;
    if(NULL == (pids = (pid_t*)malloc(sizeof(pid_t) * n)))
        ERR("malloc");

    create_children_and_pipes(n, outputPipes, mainPipe[1], pids);

    if (close(mainPipe[1]))  // Rodzic zamyka koniec do pisania (z grotem, główna strzałka)
        ERR("close");

    parent_work(n, outputPipes, mainPipe[0], pids);

    for(int i = 0; i < n; i++) {
        if(close(outputPipes[i]))
            ERR("close");
    }

    if (close(mainPipe[0]))  // Zamknięcie potoku R[0] po odczycie
        ERR("close");

    free(outputPipes); 
    free(pids);

    return EXIT_SUCCESS;
}