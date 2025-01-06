#include <linux/limits.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // do funkcji usleep, read i close
#include <string.h>
#include <stdbool.h>
#include <limits.h> // PATH_MAX
#include <fcntl.h> // do funkcji open
#include <sys/stat.h> // do funkcji lstat i sprawdzania typów plików
#include "circular_buffer.h"

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct worker_args {
    pthread_t tid;
    circular_buffer *buffer;
    bool *quitFlag;
    pthread_mutex_t *mxQuitFlag;
    int worker_id;
    int *processedFiles; // liczba przetworzonych plików txt
    int *totalFiles;     // całkowita liczba plików
    pthread_mutex_t *mxProcessed;
} worker_args_t;

void ReadArgs(int argc, char **argv, char *startPath, int *threadCount);
void explore_directory(const char* dir_path, circular_buffer *buffer, int *totalFiles);
void* worker_func(void* voidArgs);

int main(int argc, char **argv) 
{
    int threadCount;
    char startPath[20];
    ReadArgs(argc, argv, startPath, &threadCount);

    bool quitFlag = false;
    pthread_mutex_t mxQuitFlag = PTHREAD_MUTEX_INITIALIZER;

    worker_args_t *threadArgs = (worker_args_t *)malloc(sizeof(worker_args_t) * threadCount);
    if(threadArgs == NULL)
        ERR("malloc");

    circular_buffer *buffer = circular_buffer_init(&quitFlag);

    int totalFiles = 0;
    int processedFiles = 0;
    pthread_mutex_t mxProcessed = PTHREAD_MUTEX_INITIALIZER;

    // Przypisanie argumentów wątków pomocniczych
    for(int i = 0; i < threadCount; i++)
    {
        threadArgs[i].buffer = buffer;
        threadArgs[i].worker_id = i+1;
        threadArgs[i].quitFlag = &quitFlag;
        threadArgs[i].processedFiles = &processedFiles;
        threadArgs[i].totalFiles = &totalFiles;
        threadArgs[i].mxQuitFlag = &mxQuitFlag;
        threadArgs[i].mxProcessed = &mxProcessed;
    }

    for(int i = 0; i < threadCount; i++)
    {
        if(pthread_create(&threadArgs[i].tid, NULL, worker_func, &threadArgs[i]) != 0)
            ERR("Cannot create a thread");
    }

    explore_directory(startPath, buffer, &totalFiles);

    // Czekaj, aż wszystkie pliki zostaną przetworzone
    while (true) {
        pthread_mutex_lock(&mxProcessed);
        if (processedFiles == totalFiles) {
            pthread_mutex_unlock(&mxProcessed);
            break;
        }
        pthread_mutex_unlock(&mxProcessed);
        usleep(5000); // Czekanie na przetworzenie plików
    }

    // Ustawienie flagi zakończenia pracy
    pthread_mutex_lock(&mxQuitFlag);
    quitFlag = true;
    pthread_mutex_unlock(&mxQuitFlag);

    for(int i = 0; i < threadCount; i++)
    {
        if(pthread_join(threadArgs[i].tid, NULL) != 0)
            ERR("pthread join");
    }

    // Zwolnienie zasobów
    circular_buffer_deinit(buffer);
    pthread_mutex_destroy(&mxQuitFlag);
    pthread_mutex_destroy(&mxProcessed);
    free(threadArgs);

    return EXIT_SUCCESS;
}

void ReadArgs(int argc, char** argv, char *startPath, int *threadCount)
{
    if(argc != 3)
        ERR("Invalid number of arguments");

    strncpy(startPath, argv[1], strlen(argv[1])+1);
    startPath[PATH_MAX-1] = '\0';

    *threadCount = atoi(argv[2]);
    if(*threadCount < 1)
        ERR("Invalid thread count");
}

void explore_directory(const char* dir_path, circular_buffer *buffer, int *totalFiles)
{
    DIR *dir = opendir(dir_path);
    if (!dir)
        ERR("opendir");

    struct dirent *entry;
    struct stat statbuf;
    char path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Zbudowanie pełnej ścieżki do pliku/katalogu
        if (snprintf(path, PATH_MAX, "%s/%s", dir_path, entry->d_name) >= PATH_MAX)
        {
            ERR("Path length exceeds PATH_MAX");
        }

        // Sprawdzenie typu pliku za pomocą lstat
        if (lstat(path, &statbuf) == -1)
        {
            ERR("lstat");
        }

        if (S_ISDIR(statbuf.st_mode))
        {
            // Rekurencyjne przeszukiwanie podkatalogów
            explore_directory(path, buffer, totalFiles);
        }
        else if (S_ISREG(statbuf.st_mode) && strstr(entry->d_name, ".txt"))
        {
            // Plik regularny z rozszerzeniem .txt
            circular_buffer_enqueue(buffer, strdup(path));
            (*totalFiles)++;
        }
    }

    if (closedir(dir) == -1)
        ERR("closedir");
}


void* worker_func(void* voidArgs)
{
    worker_args_t* args = voidArgs;
    char* file_path;

    for(;;)
    {
        // Sprawdzanie flagi zakończenia pracy
        pthread_mutex_lock(args->mxQuitFlag);
        bool quit = *(args->quitFlag);
        pthread_mutex_unlock(args->mxQuitFlag);

        if (quit && args->buffer->count == 0)
            break;

        // Pobranie elementu z bufora
        file_path = circular_buffer_dequeue(args->buffer);
        if(file_path != NULL)
        {
            printf("Pracownik %d reprezentuje plik %s\n", args->worker_id, file_path);
            free(file_path); // zwolnienie pamięci

            pthread_mutex_lock(args->mxProcessed);
            (*args->processedFiles)++;
            pthread_mutex_unlock(args->mxProcessed);
        }
        else 
        {
            // Jeśli bufor jest pusty, wątek może chwilę poczekać
            usleep(5000); // 5ms
        }
    }

    return NULL;
}
