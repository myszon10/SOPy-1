// WYPISANIE PLIKÓW Z KATALOGÓW PODANYCH W ŚCIEŻKACH JAKO ARGUMENTY, FILTROWANIE PO ROZSZERZENIACH

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h> // funkcja strrchr

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 101

char* get_file_extention(char *filename)
{
    char* dot = strrchr(filename, '.'); // wskaźnik ostatniego wystąpienia . w filename lub NULL jeśli nie ma
    if(!dot || dot == filename) // znajduje się kropka i nie na początku
    {
        return "";
    }
    return dot + 1;
}

void scan_dir(char **extenxions, int ext_count)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;

    if((dirp = opendir(".")) == NULL) ERR("opendir");

    while((dp = readdir(dirp)) != NULL) 
    {
        errno = 0;
        if(lstat(dp->d_name, &filestat)) ERR("lstat");

        for(int i = 0; i < ext_count; i++)
        {
            if(strcmp(get_file_extention(dp->d_name), extenxions[i]) == 0)
            {
                printf("%s %ld\n", dp->d_name, filestat.st_size);
            }
        }
    }
    if(errno != 0) ERR("readdir");
    if(closedir(dirp)) ERR("closedir");
}

int main(int argc, char* argv[]) 
{
    int c;
    char path[MAX_PATH];

    if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");

    char **extensions = malloc(2 * sizeof(char*)); // tablica dynamiczna na 2 stringi
    int ext_count = 0; // aktualna liczba rozszerzeń
    int ext_size = 2; // aktualny rozmiar tablicy

    while((c = getopt(argc, argv, "p:e:")) != -1)
    {
        if(c == 'e') 
        {
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
        }
    }

    optind = 1;

    while((c = getopt(argc, argv, "p:e:")) != -1)
    {
        if(c == 'p')
        {
            if(chdir(optarg)) ERR("chdir");
            
            fprintf(stdout, "path: %s\n", optarg);
            scan_dir(extensions, ext_count);

            if(chdir(path)) ERR("chdir");
        }
    }

    for(int i = 0; i < ext_count; i++) 
        free(extensions[i]);

    free(extensions);

    return EXIT_SUCCESS;
}