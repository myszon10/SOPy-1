#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/param.h> // funkcja MIN
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
    int dogNr;
    int *dogsFinished;
    int *firstPlace, *secondPlace, *thirdPlace;
    pthread_mutex_t *mxPodium;
    pthread_mutex_t *mxFinished;
} dogArgs_t;

void ReadArguments(int argc, char **argv, int *n, int *m);
void* DogRun(void *voidArgs);
void thread_sleep(int msec);

int main(int argc, char **argv)
{
    int n, m;
    int dogsFinished = 0;
    int firstPlace = -1, secondPlace = -1, thirdPlace = -1;
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

    pthread_mutex_t mxPodium = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mxFinished = PTHREAD_MUTEX_INITIALIZER;

    srand(time(NULL));
    for(int i = 0; i < m; i++)
    {
        dogs[i].seed = rand();
        dogs[i].mxTrack = mxTrack;
        dogs[i].n = &n;
        dogs[i].track = track;
        dogs[i].dogNr = i+1;
        dogs[i].dogsFinished = &dogsFinished;
        dogs[i].firstPlace = &firstPlace;
        dogs[i].secondPlace = &secondPlace;
        dogs[i].thirdPlace = &thirdPlace;
        dogs[i].mxPodium = &mxPodium;
        dogs[i].mxFinished = &mxFinished;
    }

    for(int i = 0; i < m; i++) 
    {
        int err = pthread_create(&(dogs[i].tid), NULL, DogRun, &dogs[i]);
        if(err != 0)
            ERR("Couldn't create thread");
    }

    while(dogsFinished < m)
    {
        thread_sleep(1000);
        printf("Track: [");
        for(int i = 0; i < n; i++)
        {  
            pthread_mutex_lock(&mxTrack[i]);
            printf("%d", track[i]);
            pthread_mutex_unlock(&mxTrack[i]);
            if(i < n-1)
                printf(", ");
        }
        printf("]\n");
    }
    printf("---\n");
    printf("Final podium:\n");
    printf("1. Dog nr %d\n", firstPlace);
    printf("2. Dog nr %d\n", secondPlace);
    printf("3. Dog nr %d\n", thirdPlace);

    for(int i = 0; i < m; i++)
    {
        int err = pthread_join(dogs[i].tid, NULL);
        if(err != 0)
            ERR("Can't join with a thread");
    }

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
    int sleepTime, distance, nextPos, pos = 0;

    while(pos < *args->n-1)
    {
        sleepTime = rand_r(&args->seed) % 1321 + 200;
        thread_sleep(sleepTime);

        distance = rand_r(&args->seed) % 5 + 1;
        nextPos = MIN(pos + distance, *args->n - 1);

        pthread_mutex_lock(&args->mxTrack[nextPos]);
        if(args->track[nextPos] != 0)
        {
            printf("[Dog nr %d] waf waf waf, pos: %d\n", args->dogNr, pos);
            pthread_mutex_unlock(&args->mxTrack[nextPos]);
            continue;
        }

        pthread_mutex_lock(&args->mxTrack[pos]);
        args->track[pos]--;
        pthread_mutex_unlock(&args->mxTrack[pos]);

        args->track[nextPos]++;
        pthread_mutex_unlock(&args->mxTrack[nextPos]);
        pos = nextPos;
        
        if(pos >= *args->n-1) break;

        printf("[Dog nr %d] pos: %d, finished: %s\n", args->dogNr, pos, (pos == *args->n-1 ? "YES" : "NO"));
    }

    pthread_mutex_lock(&args->mxTrack[pos]);
    args->track[pos]--;
    pthread_mutex_unlock(&args->mxTrack[pos]);
    
    if(*args->firstPlace == -1)
    {
        pthread_mutex_lock(args->mxPodium);
        *args->firstPlace = args->dogNr;
        pthread_mutex_unlock(args->mxPodium);
    }
    else if(*args->secondPlace == -1)
    {
        pthread_mutex_lock(args->mxPodium);
        *args->secondPlace = args->dogNr;
        pthread_mutex_unlock(args->mxPodium);
    }
    else if(*args->thirdPlace == -1)
    {
        pthread_mutex_lock(args->mxPodium);
        *args->thirdPlace = args->dogNr;
        pthread_mutex_unlock(args->mxPodium);
    }

    pthread_mutex_lock(args->mxFinished);
    (*args->dogsFinished)++;
    pthread_mutex_unlock(args->mxFinished);
    printf("[Dog nr %d] finished at spot %d\n", args->dogNr, *args->dogsFinished);

    return NULL;
}

void thread_sleep(int msec)
{
    struct timespec t = {0, 0};
    if(msec >= 1000)
    {
        t.tv_sec++;
        msec -= 1000;
    }
    t.tv_nsec += 1000000 * msec;
    nanosleep(&t, NULL);
}