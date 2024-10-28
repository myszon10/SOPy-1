#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char* argv[])
{
    printf("%s pattern\n", argv[0]);
    printf("pattern - string pattern to search at standard input\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    int numberingLines = 0;
    int c = 0;

    while((c = getopt(argc, argv, "n")) != -1)
    {
        if(c == 'n') numberingLines = 1;
        else usage(argv);
    }

    // obecny indeks argumentów przekroczył liczbę argc => jest flaga bez wzorca
    if(optind >= argc) usage(argv); 
    // za mało argumentów
    if(optind + 1 < argc) usage(argv);

    char *pattern = argv[optind];

    char* line = NULL;
    size_t line_len = 0;
    int lineNr = 1;

    while (getline(&line, &line_len, stdin) != -1)  // man 3p getdelim
    {
        if (strstr(line, pattern))
        {
            if(numberingLines == 1)
            {
                printf("%d:%s\n", lineNr, line);
            }
            else
            {
                printf("%s\n", line);  // getline() should return null terminated data
            }
        }
        lineNr++;
    }

    if (line)
        free(line);


    return EXIT_SUCCESS;
}