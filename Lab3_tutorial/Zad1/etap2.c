#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SHM_SIZE 1024
#define ALF_SIZE 26

int main(int argc, char **argv) 
{
    int fd;
    if((fd = open("./file.txt", O_RDWR)) == -1)
        ERR("open");

    char *data;
    if((data = (char*)mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
        ERR("mmap");

    int char_count[ALF_SIZE] = { 0 };

    int i = 0;
    while(1)
    {
        char c = data[i++];
        if(c == '\0')
            break;

        if(c >= 'a' && c <= 'z')
            char_count[c - 'a']++;
    }

    for(int i = 0; i < ALF_SIZE; i++)
    {
        if(char_count[i] > 0)
        {
            printf("%c: %d\n", 'a' + i, char_count[i]);
        }
    }
}