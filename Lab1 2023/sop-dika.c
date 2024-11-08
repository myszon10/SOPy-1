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
#define MAXFD 20

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

void show_stage2(const char *const path, const struct stat *const stat_buf)
{
    if(S_ISREG(stat_buf->st_mode))
    {
        printf("--------\nType: file\n");
        printf("File size: %ld\n--------\n", stat_buf->st_size);

        int fd;
        if((fd = open(path, O_RDONLY)) == -1) ERR("open");

        char buffer[BUF_SIZE];
        ssize_t bytes_read;

        while((bytes_read = bulk_read(fd, buffer, BUF_SIZE)) > 0) // sprawdzanie, czy bytes_read > 0
        {
            // 1 - rozmiar jednego elementu danych, bytes_read - liczba bajtów do wpisania
            // fwrite zwraca liczbę bajtów, które zostały wpisane do strumienia
            if(fwrite(buffer, 1, bytes_read, stdout) < bytes_read)
            {
                close(fd);
                ERR("fwrite");
            }
        }
        if(bytes_read < 0) ERR("bulk_read");
        if(close(fd) == -1) ERR("close");
    }
    else if(S_ISDIR(stat_buf->st_mode))
    {
        printf("--------\nType: directory\n--------\n");
        DIR *dirp;
        struct dirent *dp;

        if((dirp = opendir(path)) == NULL) ERR("opendir");

        while((dp = readdir(dirp)) != NULL)
        {
            errno = 0;
            printf("%s\n", dp->d_name);
        }
        if(errno != 0) ERR("readdir"); // readdir zwrócił NULL, ale nie przez koniec pliku -> jakiś błąd
        if(closedir(dirp) != 0) ERR("closedir");
    }
    else
    {
        printf("Type: unknown\n");
    }
}

void write_stage3(const char *const path, const struct stat *const stat_buf) 
{
    int fd;
    if((fd = open(path, O_RDWR | O_APPEND)) == -1) ERR("open");

    char buffer[BUF_SIZE];
    ssize_t bytes_read;

    while((bytes_read = bulk_read(fd, buffer, BUF_SIZE)) > 0) 
    {
        if(fwrite(buffer, 1, bytes_read, stdout) < bytes_read)
        {
            close(fd);
            ERR("fwrite");
        }
    }
    if(bytes_read < 0) 
    {
        close(fd);
        ERR("bulk_read");
    }
    
    while(fgets(buffer, BUF_SIZE, stdin) != NULL)
    {
        if(strcmp(buffer, "\n") == 0) 
            break;

        ssize_t bytes_written = bulk_write(fd, buffer, strlen(buffer));
        if(bytes_written < 0)
        {
            close(fd);
            ERR("bulk_write");
        }
    }

    if(close(fd) == -1) ERR("close");
}

int walk(const char* path, const struct stat *s, int type, struct FTW *f) 
{
    switch(type)
    {
        case FTW_D:
        case FTW_DP: // FTW_DP - obsługa typu directory dla przeszukiwania DFS (FTW_DEPTH)
            printf("%s type: directory\n", path);
            break;
        case FTW_F:
            printf("%s type: file\n", path);
            break;
        default:
            printf("%s type: other\n", path);
    }
    return 0;
}

void walk_stage4(const char *const path, const struct stat *const stat_buf) 
{
    if(nftw(path, walk, MAXFD, FTW_PHYS | FTW_DEPTH) == -1)
    {
        ERR("nftw");
    }
}

int interface_stage1() 
{
    char buffer[BUF_SIZE];
    struct stat stats;

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
        fprintf(stderr, "File does not exist\n");
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
