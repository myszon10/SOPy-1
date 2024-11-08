#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define BUF_SIZE 256

char buffer[BUF_SIZE];
struct stat stats;


ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void show_stage2(const char *const path, const struct stat *const stat_buf) {}

void write_stage3(const char *const path, const struct stat *const stat_buf) {}

void walk_stage4(const char *const path, const struct stat *const stat_buf) {}

int interface_stage1() 
{
    printf("1. show\n2. write\n3. walk\n4. exit\n");
    
    if(fgets(buffer, BUF_SIZE, stdin) == NULL) ERR("fgets");
    
    if(strlen(buffer) != 2) // liczba i znak nowej linii 
    {
        fprintf(stderr, "Wrong input\n");
        return 1;
    }

    char opt = buffer[0];
    if(opt == '4') 
        return 0;
    else if(!('1' <= opt && opt <= '3'))
    {
        fprintf(stderr, "Wrong input\n");
        return 1;
    }
    // wczytywanie ścieżki do pliku
    if(fgets(buffer, BUF_SIZE, stdin) == NULL) ERR("fgets");

    buffer[strlen(buffer) - 1] = '\0'; // ostatni znak bufferu był \n, zamieniamy go na znak końca stringa 
    if(stat(buffer, &stats) == -1) 
    {
        fprintf(stderr, "File does not exist");
        return 1;
    }
    
    switch(opt)
    {
        case '1':
            show_stage2(buffer, &stats);
            return 1;
        case '2':
            write_stage3(buffer, &stats);
            return 1;
        case '3':
            walk_stage4(buffer, &stats);
            return 1;
    }
    return 1;
}

int main()
{
    while (interface_stage1())
        ;
    return EXIT_SUCCESS;
}
