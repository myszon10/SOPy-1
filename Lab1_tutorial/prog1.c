#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void scanDir() {
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
    printf("Files: %d, Links: %d, Directories: %d,  Other: %d", files, links, dirs, other);
}

int main(int argc, char* argv[]) {
    scanDir();
    return EXIT_SUCCESS;
}