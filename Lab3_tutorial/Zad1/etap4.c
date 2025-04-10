#define _GNU_SOURCE

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SHM_SIZE 1024
#define MAX_CHAR 256

typedef struct
{
    pthread_mutex_t mutex[MAX_CHAR];
    int count[MAX_CHAR];
} shared_memory;

void count_chars(char *buffer, ssize_t size, shared_memory *shm)
{
    for(int i = 0; i < size; i++) 
    {
        unsigned char c = buffer[i];
        pthread_mutex_lock(&shm->mutex[c]);
        shm->count[c]++;
        pthread_mutex_unlock(&shm->mutex[c]);
    }
}

void child_work(char *filename, int child_no, int child_count, shared_memory *shm)
{
    srand(getpid());
    if(rand() % 100 < 30)
    {
        fprintf(stderr, "Child %d: Simulating failure\n", child_no);
        abort();
    }

    int fd;
    if((fd = open(filename, O_RDONLY)) == -1)
        ERR("open");

    struct stat st;
    if(fstat(fd, &st) == -1)
    {
        close(fd);
        ERR("fstat");
    }

    char *buffer;
    if((buffer = (char*)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED)
        ERR("mmap");

    int chunk_size = st.st_size / child_count;
    int start = child_no * chunk_size;
    int end = (child_no == child_count - 1) ? st.st_size : start + chunk_size;

    count_chars(buffer + start, end - start, shm);

    if(close(fd))
        ERR("close");
    if(munmap(buffer, st.st_size))
        ERR("munmap");

    exit(EXIT_SUCCESS);
}

void create_children(int n, char *filename, shared_memory *shm)
{
    for(int i = 0; i < n; i++)
    {
        pid_t pid = fork();
        switch(pid)
        {
            case -1:
                ERR("fork");
            
            case 0:
                child_work(filename, i, n, shm);
                break;
        }
    }
}

/*
while(wait(NULL) > 0);
*/

int main(int argc, char **argv) 
{
    if(argc != 2)
        ERR("Invalid arguments");

    int n = atoi(argv[1]);
    if(n <= 0)
        ERR("Invalid child count");

    int shm_fd;
    char shm_name[32];
    sprintf(shm_name, "/count");

    if((shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0666)) == -1)
        ERR("shm_open");
    if(ftruncate(shm_fd, sizeof(shared_memory)) == -1)
        ERR("ftruncate");

    shared_memory *shm;
    if((shm = (shared_memory*)mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST);

    for(int i = 0; i < MAX_CHAR; i++)
    {
        shm->count[i] = 0;

        if(pthread_mutex_init(&shm->mutex[i], &mutex_attr) != 0)
            ERR("pthread_mutex_init");
    }

    create_children(n, "file.txt", shm);

    int failed = 0;
    for(int i = 0; i < n; i++)
    {
        int status;
        pid_t pid = wait(&status);
        if(pid == -1)
            ERR("wait");

        if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            fprintf(stderr, "Child %d failed (status: %d)\n", pid, status);
            failed = 1;
        }
    }

    if(failed)
    {
        fprintf(stderr, "Task failed.\n");
    }
    else
    {
        printf("Character counts:\n");
        for(int i = 0; i < MAX_CHAR; i++)
        {
            if(shm->count[i] > 0)
                printf("%c: %d\n", (char)i, shm->count[i]);
        }
    }

    for(int i = 0; i < MAX_CHAR; i++)
    {
        pthread_mutex_destroy(&shm->mutex[i]);
    }

    pthread_mutexattr_destroy(&mutex_attr);
    if(munmap(shm, sizeof(shared_memory)))
        ERR("munmap");
    if(close(shm_fd))
        ERR("close");

    shm_unlink(shm_name);
}