#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

void read_from_fifo(int fifo)
{
    ssize_t count;
    char c;
    do
    {
        // Próba odczytu pojedynczego znaku z FIFO
        if ((count = read(fifo, &c, 1)) < 0)
            ERR("read"); // Obsługa błędu odczytu
        
        // Jeśli odczytano poprawny znak i jest on alfanumeryczny, wypisz go
        if (count > 0 && isalnum(c))
            printf("%c", c);
    } while (count > 0); // Powtarzaj dopóki są dane w FIFO
}

int main(int argc, char **argv)
{
    int fifo;
    
    // Sprawdzenie poprawności liczby argumentów
    if (argc != 2)
        usage(argv[0]);

    // Tworzenie FIFO, jeśli nie istnieje
    if (mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
        if (errno != EEXIST) // Ignorujemy błąd, jeśli FIFO już istnieje
            ERR("create fifo");

    // Otwieramy FIFO w trybie tylko do odczytu
    if ((fifo = open(argv[1], O_RDONLY)) < 0)
        ERR("open");
    
    // Czytamy dane z FIFO i filtrujemy niealfanumeryczne znaki
    read_from_fifo(fifo);
    
    // Zamykamy FIFO po zakończeniu odczytu
    if (close(fifo) < 0)
        ERR("close fifo:");
    
    return EXIT_SUCCESS; // Program zakończony poprawnie
}