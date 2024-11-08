// WYPISANIE NAZW I ROZMIARÓW PLIKÓW W KATALOGU ROBOCZYM

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define MAX_PATH 101
#define FILE_BUF_LEN 256

ssize_t scan_dir(char* buff)
{
    int fd = open(".", O_RDONLY);
    ssize_t c;

    for(;;)
    {
        c = TEMP_FAILURE_RETRY(read(fd, buff, FILE_BUF_LEN));
        if(c < 0)
            return c;
        if(c == 0)
            return 0;
    }
    
}

int make_file(char* name)
{
    int fd = open(name, O_WRONLY | O_CREAT, 0777);
    if(fd == -1) ERR("open");

    return fd; 
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
            
            case 'o':
                out = make_file(optarg);
        }
    }
    return EXIT_SUCCESS;
}