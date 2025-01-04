#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
//#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct dogArgs
{
    pthread_t tid;
    UINT seed;
    pthread_mutex_t *mxTrack;
    int *track;
    int *n;
} dogArgs_t;

void ReadArguments(int argc, char **argv, int *n, int *m);
void* DogRun(void *voidArgs);

int main(int argc, char **argv)
{
    int n, m;
    ReadArguments(argc, argv, &n, &m);

    int *track = malloc(sizeof(int) * n);
    if(track == NULL)
        ERR("malloc");

    pthread_mutex_t *mxTrack = malloc(sizeof(pthread_mutex_t) * n);
    if(mxTrack == NULL)
        ERR("malloc");

    dogArgs_t *dogs = malloc(sizeof(dogArgs_t) * m);
    if(dogs == NULL)
        ERR("malloc");

    for(int i = 0; i < n; i++)
    {
        track[i] = 0;
        if (pthread_mutex_init(&mxTrack[i], NULL))
            ERR("Couldn't initialize mutex!");
    }

    srand(time(NULL));
    for(int i = 0; i < m; i++)
    {
        dogs[i].seed = rand();
        dogs[i].mxTrack = mxTrack;
        dogs[i].n = &n;
        dogs[i].track = track;
    }

    for(int i = 0; i < m; i++) 
    {
        int err = pthread_create(&(dogs[i].tid), NULL, DogRun, &dogs[i]);
        if(err != 0)
            ERR("Couldn't create thread");
    }

    for(int i = 0; i < m; i++)
    {
        int err = pthread_join(dogs[i].tid, NULL);
        if(err != 0)
            ERR("Can't join with a thread");
    }

    printf("Final track:\n[");
    for(int i = 0; i < n; i++)
    {
        printf("%d", track[i]);
        if(i < n-1)
            printf(", ");
    }
    printf("]\n");

    for (int i = 0; i < n; i++) 
    {
        pthread_mutex_destroy(&mxTrack[i]);
    }

    free(track);
    free(mxTrack);
    free(dogs);
}

void ReadArguments(int argc, char **argv, int *n, int *m)
{
    if(argc != 3)
        ERR("Invalid number of arguments");

    *n = atoi(argv[1]);
    *m = atoi(argv[2]);

    if(*n <= 20 || *m <= 2)
        ERR("Invalid argument values");
}

void* DogRun(void *voidArgs)
{
    dogArgs_t *args = voidArgs;
    int ind = rand_r(&args->seed) % *args->n;

    pthread_mutex_lock(&args->mxTrack[ind]);
    args->track[ind]++;
    pthread_mutex_unlock(&args->mxTrack[ind]);

    return NULL;
}