// WYPISANIE NAZW I ROZMIARÓW PLIKÓW W KATALOGU ROBOCZYM

//#include <bits/getopt_core.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define MAX_PATH 101


void scan_dir(FILE* buff)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;

    if((dirp = opendir(".")) == NULL) ERR("opendir");

    while((dp = readdir(dirp)) != NULL) 
    {
        errno = 0;
        if(lstat(dp->d_name, &filestat)) ERR("lstat");

        fprintf(buff, "%s %ld\n", dp->d_name, filestat.st_size);

    }
    if(errno != 0) ERR("readdir");
    if(closedir(dirp)) ERR("closedir");
}

FILE* make_file(char* name)
{
    FILE* s1;
    umask(~0777);
    if((s1 = fopen(name, "w+")) == NULL) ERR("fopen");
    return s1;
}

int main(int argc, char* argv[]) 
{
    int c;
    char path[MAX_PATH];
    FILE* out = NULL;
    FILE* buff;
    if(getcwd(path, MAX_PATH) == NULL) ERR("getcwd");

    while((c = getopt(argc, argv, "p:o:")) != -1)
    {
        switch(c)
        {
            case 'o':
                out = make_file((optarg));
                buff = out;
                break;
        }
    }
    optind = 1;

    while((c = getopt(argc, argv, "p:o:")) != -1) 
    {
        switch(c)
        {
            case 'p':
                if(chdir(optarg)) ERR("chdir");

                if(out == NULL) 
                    buff = stdout;
                else
                    buff = out;
                
                fprintf(buff, "SCIEZKA:\n%s\n", optarg);
                scan_dir(buff);

                if(chdir(path)) ERR("chdir");
                break;
            
            // case 'o':
            //     out = make_file(optarg);
        }
    }
    return EXIT_SUCCESS;
}