#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MSG_SIZE (PIPE_BUF - sizeof(pid_t)) // Maksymalna ilość danych w pakiecie (reszta na PID)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file file\n", name);
    exit(EXIT_FAILURE);
}

void write_to_fifo(int fifo, int file)
{
    int64_t count;
    char buffer[PIPE_BUF]; // Bufor o rozmiarze PIPE_BUF
    char *buf;
    *((pid_t *)buffer) = getpid(); // Zapisujemy PID na początku bufora
    buf = buffer + sizeof(pid_t); // Bufor na dane (po PID)

    do
    {
        if ((count = read(file, buf, MSG_SIZE)) < 0)
            ERR("Read:");

        if (count < MSG_SIZE)
            memset(buf + count, 0, MSG_SIZE - count); // Uzupełniamy brakujące bajty zerami

        if (count > 0)
            if (write(fifo, buffer, PIPE_BUF) < 0) 
                ERR("Write:");
    } while (count == MSG_SIZE); // Dopóki plik jeszcze ma dane do wysłania
}

int main(int argc, char **argv)
{
    int fifo, file;
    if (argc != 3)
        usage(argv[0]);

    // Tworzymy FIFO, jeśli nie istnieje
    if (mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
        if (errno != EEXIST) // Jeśli FIFO już instnieje, to ignorujemy błąd
            ERR("create fifo");
    
    // Otwieramy plik wejściowy w trybie do zapisu
    if ((fifo = open(argv[1], O_WRONLY)) < 0)
        ERR("open");

    // Otwieramy plik wejściowy w trybie do odczytu
    if ((file = open(argv[2], O_RDONLY)) < 0)
        ERR("file open");

    // Wysyłamy dane z pliku do FIFO
    write_to_fifo(fifo, file);

    // Zamykamy plik wejściowy
    if (close(file) < 0)
        perror("Close fifo:");
    
    // Zamykamy FIFO
    if (close(fifo) < 0)
        perror("Close fifo:");
    return EXIT_SUCCESS;
}