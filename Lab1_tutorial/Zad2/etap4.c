// WYPISANIE PLIKÓW Z KATALOGÓW PODANYCH W ŚCIEŻKACH JAKO ARGUMENTY, FILTROWANIE PO ROZSZERZENIACH
// WYSZUKIWANIE REKURENCYJNE Z PODANĄ W ARGUMENCIE GŁĘBOKOŚCIĄ

#define _XOPEN_SOURCE 500

#include <dirent.h>
//#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h> // funkcja strrchr
#include <ftw.h> // struct FTW

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 101
#define MAXFD 20

char **extensions;
int ext_count;
int ext_size;
int depth;
FILE* buff;

char* get_file_extention(const char *filename)
{
    char* dot = strrchr(filename, '.'); // wskaźnik ostatniego wystąpienia . w filename lub NULL jeśli nie ma
    if(!dot || dot == filename) // znajduje się kropka i nie na początku
    {
        return "";
    }
    return dot + 1;
}

int walk(const char* name, const struct stat *s, int type, struct FTW *f)
{
    if(f->level > depth) return 0;

    for(int i = 0; i < ext_count; i++)
    {
        if(strcmp(get_file_extention(name), extensions[i]) == 0)
        {
            fprintf(buff, "%s %ld\n", name, s->st_size);
        }
    }
    return 0;
}

FILE* make_file(const char* name)
{
    FILE* s1;
    umask(~0777);
    if((s1 = fopen(name, "w+")) == NULL) ERR("fopen");
    return s1;
}

int main(int argc, char* argv[]) 
{
    int c;
    depth = 1;
    char path[MAX_PATH]; // ścieżka do przeszukania
    buff = stdout;
    
    const char *var_name = "L1_OUTPUTFILE";
    const char *var_value = "/tmp/L1_output.txt";

    // tworzenie stringa NAZWA=ścieżka dla putenv
    char *env_var = malloc(strlen(var_name) + strlen(var_value) + 2); // 2 znaki \0
    if(env_var == NULL) ERR("malloc");
    sprintf(env_var, "%s=%s", var_name, var_value);


    if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");

    extensions = malloc(2 * sizeof(char*)); // tablica dynamiczna na 2 stringi
    ext_count = 0; // aktualna liczba rozszerzeń
    ext_size = 2; // aktualny rozmiar tablicy

    while((c = getopt(argc, argv, "p:e:d:o")) != -1)
    {
        switch(c) 
        {
            case 'e':
                // dodanie nowego rozszerzenia do tablicy extensions
                if(ext_count >= ext_size)
                {
                    ext_size *= 2; // *= 2 zamiast ++, optymalizacja czasowa i pamięciowa
                    char **tmp = realloc(extensions, ext_size * sizeof(char*));
                    if(tmp == NULL) // realloc się nie powiodło
                    {
                        for(int i = 0; i < ext_count; i++) 
                            free(extensions[i]);
                        free(extensions);
                        ERR("realloc");
                    }
                    extensions = tmp;
                }
                extensions[ext_count++] = strdup(optarg);
                break;
            case 'd':
                depth = atoi(optarg);
                break;
            case 'o':
                if(putenv(env_var) != 0)
                {
                    free(env_var);
                    ERR("putenv");
                }
                buff = make_file(var_value);
        }
    }

    optind = 1;

    while((c = getopt(argc, argv, "p:e:d:o")) != -1)
    {
        if(c == 'p')
        {
            if(chdir(optarg)) ERR("chdir");
            
            fprintf(buff, "path: %s\n", optarg);

            if(nftw(optarg, walk, MAXFD, FTW_PHYS | FTW_DEPTH) == -1)
            {
                ERR("nftw");
            }

            if(chdir(path)) ERR("chdir");
        }
    }

    for(int i = 0; i < ext_count; i++) 
        free(extensions[i]);

    free(extensions);
    // nie zwalniamy ręcznie env_var, bo może to skutkować straceniem zmiennej L1_OUTPUTFILE
    //free(env_var);

    if(buff != stdout)
    {
        char *output_path = getenv("L1_OUTPUTFILE");
        if(output_path) 
        {
            printf("L1_OUTPUTFILE = %s\n", output_path);
        }
        else 
        {
            ERR("L1_OUTPUTFILE not set");
        }
        fclose(buff);
    }

    return EXIT_SUCCESS;
}

/*
Nie wiedziałem dokładnie o co chodzi z ustawieniem zmiennej środowiskowej, bo nie da się tylko z poziomu kodu C
ustawić globalnej zmiennej środowiskowej, zmienna ta istnieje tylko na czas działania procesu. Program tworzy
za to plik /tmp/L1_output.txt który można zcatować po wykonaniu programu, a końcówka kodu pokazuje, że na 
czas działania programu zmienna środowiskowa L1_OUTPUTFILE ma ustawioną ścieżkę /tmp/L1_output.txt
*/