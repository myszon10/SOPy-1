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

int main(int argc, char **argv) 
{
    int fd;
    if((fd = open("./file.txt", O_RDWR)) == -1)
        ERR("open");

    char *data;
    if((data = (char*)mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
        ERR("mmap");

    printf("%s\n", data);
}