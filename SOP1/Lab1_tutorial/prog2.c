// ZLICZANIE PLIKÓW/KATALOGÓW/LINKÓW/INNYCH W KATALOGU OKREŚLONYM ŚCIEŻKĄ

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH 101

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void scan_dir() {
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;

    if((dirp = opendir(".")) == NULL) {
        ERR("opendir");
    }

    int files = 0, links = 0, dirs = 0, other = 0;

    while((dp = readdir(dirp)) != NULL) {
        errno = 0;
        if(lstat(dp->d_name, &filestat)) {
            ERR("lstat");
        }
        if(S_ISDIR(filestat.st_mode)) {
            dirs++;
        }
        else if(S_ISREG(filestat.st_mode)) {
            files++;
        }
        else if(S_ISLNK(filestat.st_mode)) {
            links++;
        }
        else {
            other++;
        }
    }

    if(errno != 0) {
        ERR("readdir");
    }
    if(closedir(dirp)) {
        ERR("closedir");
    }
    printf("Files: %d, Links: %d, Directories: %d,  Other: %d\n", files, links, dirs, other);
}

int main(int argc, char* argv[]) {
    char path[MAX_PATH];
    if(getcwd(path, MAX_PATH) == NULL) { // pobranie ścieżki do bierzącego katalogu
        ERR("getcwd");
    }

    for(int i = 1; i < argc; i++) {
        if(chdir(argv[i])) {
            ERR("chdir");
        }
        printf("%s\n", argv[i]);
        scan_dir();
        if(chdir(path)) {
            ERR("chdir");
        }
    }
    return EXIT_SUCCESS;
}