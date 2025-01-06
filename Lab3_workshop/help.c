#include <bits/types/sigset_t.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define BUF_SIZE 15
typedef unsigned int UINT;
typedef struct timespec timespec_t;
// circ buff
typedef struct buffer
{
    char * tab[BUF_SIZE];
    int count;
    int head, tail;
    pthread_mutex_t * pmxBuf;
} buffer_t;

typedef struct threadArgs
{
    pthread_t tid;
    buffer_t * filepathBuffer;
    int * counters;
    pthread_mutex_t * pmxCounters;
    bool * finished_scanning;
    bool * quitFlag;
    pthread_mutex_t * mxQuitFlag;
    sigset_t * mask;
} threadArgs_t;

void msleep(UINT milisec)
{
    time_t sec = (int)(milisec / 1000);
    milisec = milisec - (sec * 1000);
    timespec_t req = {0};
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if (nanosleep(&req, &req))
        ERR("nanosleep");
}

buffer_t * init()
{
    buffer_t * buffer = (buffer_t *)malloc(sizeof(buffer_t));
    if(buffer == NULL) ERR("malloc");
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->pmxBuf = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if(buffer->pmxBuf == NULL) ERR("malloc");
    if(pthread_mutex_init(buffer->pmxBuf, NULL)) ERR("Cant init mutex");
    return buffer;
}

char * deque(buffer_t * buf)
{
    char * ptr = NULL;
    pthread_mutex_lock(buf->pmxBuf);
    if(buf->count == 0)
    {
        pthread_mutex_unlock(buf->pmxBuf);
        return ptr;
    }
    ptr = buf->tab[buf->tail];
    buf->tail = (buf->tail + 1) % BUF_SIZE;
    buf->count--;
    pthread_mutex_unlock(buf->pmxBuf);
    return ptr;
}

void enque(buffer_t * buf, char * filepath)
{
    while(1)
    {
        pthread_mutex_lock(buf->pmxBuf);
        if(buf->count == BUF_SIZE)
        {
            pthread_mutex_unlock(buf->pmxBuf);
            msleep(5);
            continue;
        }
        buf->tab[buf->head] = filepath;
        buf->head = (buf->head + 1) % BUF_SIZE;
        buf->count++;
        pthread_mutex_unlock(buf->pmxBuf);
        break;
    }
}

void deinit(buffer_t * buff)
{
    pthread_mutex_lock(buff->pmxBuf);
    char * filepath;
    while(buff->count != 0)
    {
        printf("%d\n", buff->count);
        pthread_mutex_unlock(buff->pmxBuf);
        filepath = deque(buff);
        free(filepath);
        pthread_mutex_lock(buff->pmxBuf);
    }
    pthread_mutex_unlock(buff->pmxBuf);
    free(buff->pmxBuf);
    free(buff);
}

void print_alphabet_counters(int * counters, pthread_mutex_t* pmxCounters)
{
    for(int i = 0; i < 'z'-'a'+1; i++)
    {
        printf("%c: %d\t", 'a' + i, counters[i]);
    }
    printf("\n");
}

void* signal_handler_thread(void* voidArgs)
{
    threadArgs_t * args = voidArgs;

    sigset_t * mask = args->mask;
    while(!*(args->quitFlag))
    {
        int sig;
        if(sigwait(mask, &sig) != 0)
            ERR("sigwait");

        if(sig == SIGUSR1)
            print_alphabet_counters(args->counters, args->pmxCounters);
        else if(sig == SIGINT)
        {
            pthread_mutex_lock(args->mxQuitFlag);
            *(args->quitFlag) = true;
            pthread_mutex_unlock(args->mxQuitFlag);
            break;
        }
    }

    return NULL;
}

void * thread_work(void * args)
{
    threadArgs_t * data = (threadArgs_t *)args;
    while(!*data->quitFlag)
    {
        char * filepath = deque(data->filepathBuffer);
        if(filepath == NULL)
        {
            if(*data->finished_scanning == true)
            {
                return NULL;
            }
            msleep(5);
            continue;
        }
        printf("[%lu] Got file %s\n", pthread_self(), filepath);
        int fd = open(filepath, O_RDONLY);
        if(fd < 0) ERR("open");
        ssize_t count;
        char a;
        while((count = read(fd, &a, 1)) > 0)
        {
            if(islower(a))
            {
                // printf("[%lu] Processing %c\n", pthread_self(), a);
                pthread_mutex_lock(&data->pmxCounters[a-'a']);
                data->counters[a - 'a']++;
                pthread_mutex_unlock(&data->pmxCounters[a-'a']);
                msleep(100);
            }
        }
        if(count < 0) ERR("read");
        if(close(fd)) ERR("close");
        free(filepath);
    }
    printf("[%lu] Quitting...\n", pthread_self());
    return NULL;
}

void scan_dir(char * dirname, buffer_t * filepathBuffer)
{
    // Działa tylko wtedy jeśli są same foldery a w jakimś folderze jest max 1 plik
    DIR * dir = opendir(dirname);
    if(NULL == dir) ERR("opendir");
    struct dirent * direntry;
    struct stat statbuf;
    errno = 0;
    char * path = (char *)malloc(256*sizeof(char));
    char * path_to_queue;
    if(NULL == path) ERR("malloc");
    while(1)
    {
        if((direntry = readdir(dir)) == NULL)
        {
            if(errno != 0) ERR("readdir");
            printf("Finished scanning data1/%s\n", dirname);
            break;
        }
        if(snprintf(path, 256,  "%s/%s", dirname, direntry->d_name) < 0) ERR("snprintf");
        printf("%s\n", path);
        if(lstat(path, &statbuf)) ERR("lstat");
        if(S_ISDIR(statbuf.st_mode))
        {
            if(strcmp(direntry->d_name, ".") != 0 && strcmp(direntry->d_name, "..") != 0)
            {
                scan_dir(path, filepathBuffer);
            }
        }
        else if(S_ISREG(statbuf.st_mode))
        {
            path_to_queue = (char *)malloc(256*sizeof(char));
            snprintf(path_to_queue, 256, "%s", path);
            enque(filepathBuffer, path_to_queue);
        }
    }
    if(closedir(dir))
    ERR("closedir");
    free(path);
}
void usage()
{
    fprintf(stderr, "Need to give number of threads on input as first arg.\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char ** argv)
{
    if(argc != 2) usage();
    int k = atoi(argv[1]);
    if(k < 0) ERR("atoi");

    threadArgs_t * args = (threadArgs_t *)malloc((k+1)*sizeof(threadArgs_t));
    if(args == NULL) ERR("args");
    int * counters = (int *)calloc(('z'-'a'+1), sizeof(int));
    if(counters == NULL) ERR("malloc"); 
    buffer_t * filepathBuffer = init();
    bool finished_scanning = false;
    pthread_mutex_t pmxBuf = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t * pmxCounters = (pthread_mutex_t *)malloc(('z'-'a'+1)*sizeof(pthread_mutex_t));
    if(pmxCounters == NULL) ERR("malloc");
    pthread_mutex_t mxQuitFlag = PTHREAD_MUTEX_INITIALIZER;
    bool quitFlag = false;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    for(int i = 0; i < 'z'-'a'+1; i++)
    {
        if(pthread_mutex_init(&pmxCounters[i], NULL))
        {
            ERR("mutex init");
        }
    }


    if(NULL == pmxCounters) ERR("malloc");

    args[k].mxQuitFlag = &mxQuitFlag;
    args[k].quitFlag = &quitFlag;
    args[k].counters = counters;
    args[k].pmxCounters = pmxCounters;
    args[k].mask = &mask;

    if(pthread_create(&args[k].tid, NULL, signal_handler_thread, &args[k]))
    ERR("pthread create");

    for(int i = 0; i < k; i++)
    {
        args[i].counters = counters;
        args[i].filepathBuffer = filepathBuffer;
        args[i].finished_scanning = &finished_scanning;
        args[i].pmxCounters = pmxCounters;
        args[i].mxQuitFlag = &mxQuitFlag;
        args[i].quitFlag = &quitFlag;
        if(pthread_create(&args[i].tid, NULL, thread_work, args)) ERR("pthread create");
    }
    
    scan_dir("./data1", filepathBuffer);
    finished_scanning = true;

    while(!quitFlag)
    {
        pthread_mutex_lock(&pmxBuf);
        if(filepathBuffer->count == 0)
        {
            pthread_mutex_unlock(&pmxBuf);
            break;
        }
        pthread_mutex_unlock(&pmxBuf);
        kill(0, SIGUSR1);
        msleep(100);
    }

    for(int i = 0; i < k; i++)
    {
        if(pthread_join(args[i].tid, NULL)) ERR("cant join");
    }

    printf("Wyniki: \n");
    quitFlag = true;
    kill(0, SIGUSR1);
    if(pthread_join(args[k].tid, NULL)) ERR("cant join");
    free(args);
    free(counters);
    free(pmxCounters);
    deinit(filepathBuffer);
    return 0;
}