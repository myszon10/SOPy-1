#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_THREADCOUNT 10

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void ReadArguments(int argc, char **argv, int *threadCount);
void* thread_counter();

int main(int argc, char **argv) 
{
    int threadCount;
    ReadArguments(argc, argv, &threadCount);
    
    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * threadCount);
    if(threads == NULL)
        ERR("Malloc error for thread arguments");

    for(int i = 0; i < threadCount; i++) 
    {
        int err = pthread_create(&threads[i], NULL, thread_counter, NULL);
        if(err != 0)
            ERR("Couldn't create thread");
    }

    for(int i = 0; i < threadCount; i++) 
    {
        int err = pthread_join(threads[i], NULL);
        if(err != 0)
            ERR("Can't join with a thread");
    }
    free(threads);
}

void ReadArguments(int argc, char **argv, int *threadCount) 
{
    *threadCount = DEFAULT_THREADCOUNT;
    if(argc == 2)
    {
        *threadCount = atoi(argv[1]);
        if(*threadCount <= 0) 
        {
            printf("Invalid value for threadCount\n");
            exit(EXIT_FAILURE);
        }
    }
    if(argc > 2)
    {
        printf("Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
}

void* thread_counter()
{
    printf("*\n");
    return NULL;
}
