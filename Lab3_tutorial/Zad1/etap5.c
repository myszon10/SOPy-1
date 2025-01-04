#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define DEFAULT_THREADCOUNT 10

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct thread_args
{
    pthread_t tid;
    UINT seed;
    int M;
    int *L;
    int *checkedThreads;
    pthread_mutex_t *mxChecked;
    bool *pQuitFlag;
    pthread_mutex_t *mxQuitFlag;
} threadArgs_t;

typedef struct signal_handler_args
{
    pthread_t tid;
    bool *pQuitFlag;
    pthread_mutex_t *mxQuitFlag;
    sigset_t *pMask;
} sigHandlerArgs_t;

void ReadArguments(int argc, char **argv, int *threadCount);
void* thread_counter(void* voidArgs);
void thread_sleep();
void* signal_handling(void *voidArgs);

int main(int argc, char **argv) 
{
    int threadCount;
    int L = 1;
    bool quitFlag = false;
    ReadArguments(argc, argv, &threadCount);

    int checkedThreads = 0;
    pthread_mutex_t mxChecked = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mxQuitFlag = PTHREAD_MUTEX_INITIALIZER;
    
    threadArgs_t *threadArgs = (threadArgs_t*)malloc(sizeof(threadArgs_t) * threadCount);
    if(threadArgs == NULL)
        ERR("Malloc error for thread arguments");

    srand(time(NULL));
    for(int i = 0; i < threadCount; i++)
    {
        threadArgs[i].seed = rand();
        threadArgs[i].M = rand_r(&threadArgs[i].seed) % 99 + 2;
        threadArgs[i].L = &L;
        threadArgs[i].checkedThreads = &checkedThreads;
        threadArgs[i].mxChecked = &mxChecked;
        threadArgs[i].pQuitFlag = &quitFlag;
        threadArgs[i].mxQuitFlag = &mxQuitFlag;
    }

    sigset_t oldMask, newMask;
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask))
        ERR("SIG_BLOCK error");

    sigHandlerArgs_t sigArgs = {
        .pQuitFlag = &quitFlag,
        .mxQuitFlag = &mxQuitFlag,
        .pMask = &newMask
    };

    if (pthread_create(&sigArgs.tid, NULL, signal_handling, &sigArgs))
        ERR("Couldn't create signal handling thread!");

    for(int i = 0; i < threadCount; i++) 
    {
        int err = pthread_create(&(threadArgs[i].tid), NULL, thread_counter, &threadArgs[i]);
        if(err != 0)
            ERR("Couldn't create thread");
    }

    for(;;)
    {
        pthread_mutex_lock(&mxQuitFlag);
        if(quitFlag == true)
        {
            pthread_mutex_unlock(&mxQuitFlag);
            break;
        }
        pthread_mutex_unlock(&mxQuitFlag);

        pthread_mutex_lock(&mxChecked);
        if(checkedThreads == threadCount)
        {
            checkedThreads = 0;
            L++;
        }
        checkedThreads = 0;
        L++;
        pthread_mutex_unlock(&mxChecked);

        thread_sleep();
    }

    for(int i = 0; i < threadCount; i++) 
    {
        int err = pthread_join(threadArgs[i].tid, NULL);
        if(err != 0)
            ERR("Can't join with a thread");
    }
    printf("\n");
    free(threadArgs);
    if (pthread_sigmask(SIG_UNBLOCK, &newMask, &oldMask))
        ERR("SIG_BLOCK error");

    pthread_mutex_destroy(&mxChecked);
    pthread_mutex_destroy(&mxQuitFlag);

    exit(EXIT_SUCCESS);
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

void* thread_counter(void *voidArgs)
{
    threadArgs_t *args = (threadArgs_t *) voidArgs;

    for(;;)
    {
        pthread_mutex_lock(args->mxQuitFlag);
        if(*(args->pQuitFlag) == true)
        {
            pthread_mutex_unlock(args->mxQuitFlag);
            return NULL;
        }
        pthread_mutex_unlock(args->mxQuitFlag);

        if(*(args->L) % args->M == 0)
        {
            printf("%d jest podzielne przez %d\n", *(args->L), args->M);
            fflush(stdout);
        }

        pthread_mutex_lock(args->mxChecked);
        (*args->checkedThreads)++;
        pthread_mutex_unlock(args->mxChecked);

        thread_sleep();
    }

    return NULL;
}

void thread_sleep()
{
    struct timespec t = {0, 100 * 1000000};
    nanosleep(&t, NULL);
}

void* signal_handling(void *voidArgs)
{
    sigHandlerArgs_t *args = voidArgs;
    int signo;
    for(;;)
    {
        if (sigwait(args->pMask, &signo))
            ERR("sigwait failed.");
        switch (signo)
        {
            case SIGINT:
                pthread_mutex_lock(args->mxQuitFlag);
                *args->pQuitFlag = true;
                pthread_mutex_unlock(args->mxQuitFlag);
                return NULL;
            default:
                printf("unexpected signal %d\n", signo);
                exit(1);
        }
    }
}