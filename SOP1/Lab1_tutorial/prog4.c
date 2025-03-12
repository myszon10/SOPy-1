// TWORZENIE PLIKU O OKREŚLONEJ W ARGUMENTACH NAZWIE, UPRAWNIENIACH I ROZMIARZE
// WYPEŁNIANIE PLIKU W OK. 10% LOSOWYMI ZNAKAMI [A-Z] I ZNAKAMI O KODZIE 0 W RESZCZIE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <bits/getopt_core.h> // niepotrzebne ale IDE nie widziało optarg

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s -n Name -p OCTAL -s SIZE\n", pname);
    exit(EXIT_FAILURE);
}

void make_file(char* name, ssize_t size, mode_t perms, int percent)
{
    FILE *s1;
    umask(~perms & 0777); // 0 na początku -> system ósemkowy, umask chcę negację docelowych uprawnień
    if((s1 = fopen(name, "w+")) == NULL) ERR("fopen");

    for(int i = 0; i < (size * percent)/100; i++) // pętla wykona się tyle razy ile liter musimy wpisać
    {
        if(fseek(s1, rand() % size, SEEK_SET)) // rand() % size jest offsetem, jeśli nie zwróci 0 to err
        {
            ERR("fseek");
        }
        fprintf(s1, "%c", 'A' + (i % ('Z' - 'A' + 1)));
    }
    if(fclose(s1)) ERR("fclose");
}

int main(int argc, char* argv[])
{
    int c;
    char* name = NULL;
    mode_t perms = -1;
    ssize_t size = -1;

    while((c = getopt(argc, argv, "n:p:s:")) != -1)
    {
        switch(c)
        {
            case 'n':
                name = optarg;
                break;
            case 'p':
                perms = strtol(optarg, (char**)NULL, 8);
                break;
            case 's':
                size = strtol(optarg, (char**)NULL, 10);
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }
    if((NULL == name) || (-1 == size) || (-1 == perms)) usage(argv[0]);
    if(unlink(name) && errno != ENOENT) // errno != ENOENT - nie ma błędu, bo plik nie istniał
    {
        ERR("unlink");
    }
    srand(time(NULL));
    make_file(name, size, perms, 10);
    return EXIT_SUCCESS;
}

