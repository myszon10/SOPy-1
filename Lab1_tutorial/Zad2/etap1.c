// WYPISANIE PLIKÓW Z KATALOGÓW PODANYCH W ŚCIEŻKACH JAKO ARGUMENTY

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 101

void scan_dir()
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;

    if((dirp = opendir(".")) == NULL) ERR("opendir");

    while((dp = readdir(dirp)) != NULL) 
    {
        errno = 0;
        if(lstat(dp->d_name, &filestat)) ERR("lstat");

        printf("%s %ld\n", dp->d_name, filestat.st_size);
    }
    if(errno != 0) ERR("readdir");
    if(closedir(dirp)) ERR("closedir");
}

int main(int argc, char* argv[]) 
{
    int c;
    char path[MAX_PATH];

    if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");

    while((c = getopt(argc, argv, "p:")) != -1)
    {
        if(c == 'p')
        {
            if(chdir(optarg)) ERR("chdir");
            
            fprintf(stdout, "path: %s\n", optarg);
            scan_dir();

            if(chdir(path)) ERR("chdir");
        }
    }

    return EXIT_SUCCESS;
}