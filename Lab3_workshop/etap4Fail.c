#include <linux/limits.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>
#include "circular_buffer.h"

#define MUTEX_COUNT 52

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

typedef struct signal_handler_args {
    int *alphabetCounter;
    pthread_mutex_t *mutexes;
    pthread_mutex_t *mxQuitFlag;
    bool *quitFlag;
    sigset_t *mask; // Wskaźnik na maskę sygnałów
} signal_handler_args_t;

typedef struct worker_args {
    pthread_t tid;
    circular_buffer *buffer;
    bool *quitFlag;
    pthread_mutex_t *mxQuitFlag;
    int worker_id;
    int *processedFiles;
    int *totalFiles;
    pthread_mutex_t *mxProcessed;
    int *alphabetCounter;
    pthread_mutex_t *mutexes;
    pthread_mutex_t *mxPrint;
} worker_args_t;

void ReadArgs(int argc, char **argv, char *startPath, int *threadCount);
void explore_directory(const char* dir_path, circular_buffer *buffer, int *totalFiles);
void* worker_func(void* voidArgs);
void process_file(const char* file_path, int *alphabetCounter, pthread_mutex_t *mutexes, int worker_id, pthread_mutex_t *mxPrint);
void* signal_handler_thread(void* voidArgs);
void print_alphabet_counters(int *alphabetCounter, pthread_mutex_t *mutexes);

int main(int argc, char **argv) 
{
    int threadCount;
    char startPath[20];
    int alphabetCounter[52] = {0};
    pthread_mutex_t mutexes[MUTEX_COUNT];
    pthread_mutex_t mxPrint = PTHREAD_MUTEX_INITIALIZER;

    for (int i = 0; i < MUTEX_COUNT; i++) {
        if (pthread_mutex_init(&mutexes[i], NULL) != 0)
            ERR("Setup of letter mutexes");
    }
    
    ReadArgs(argc, argv, startPath, &threadCount);

    bool quitFlag = false;
    pthread_mutex_t mxQuitFlag = PTHREAD_MUTEX_INITIALIZER;

    worker_args_t *threadArgs = (worker_args_t *)malloc(sizeof(worker_args_t) * threadCount);
    if (threadArgs == NULL)
        ERR("malloc");

    circular_buffer *buffer = circular_buffer_init(&quitFlag);

    int totalFiles = 0;
    int processedFiles = 0;
    pthread_mutex_t mxProcessed = PTHREAD_MUTEX_INITIALIZER;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);

    // Blokujemy sygnały w wątku głównym i innych wątkach
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
        ERR("pthread_sigmask");

    signal_handler_args_t signalArgs = {
        .alphabetCounter = alphabetCounter,
        .mutexes = mutexes,
        .mxQuitFlag = &mxQuitFlag,
        .quitFlag = &quitFlag,
        .mask = &mask
    };

    pthread_t signalThread;
    if (pthread_create(&signalThread, NULL, signal_handler_thread, &signalArgs) != 0)
        ERR("Cannot create signal handler thread");

    for (int i = 0; i < threadCount; i++) {
        threadArgs[i].buffer = buffer;
        threadArgs[i].worker_id = i + 1;
        threadArgs[i].quitFlag = &quitFlag;
        threadArgs[i].processedFiles = &processedFiles;
        threadArgs[i].totalFiles = &totalFiles;
        threadArgs[i].mxQuitFlag = &mxQuitFlag;
        threadArgs[i].mxProcessed = &mxProcessed;
        threadArgs[i].alphabetCounter = alphabetCounter;
        threadArgs[i].mutexes = mutexes;
        threadArgs[i].mxPrint = &mxPrint;
    }

    for (int i = 0; i < threadCount; i++) {
        if (pthread_create(&threadArgs[i].tid, NULL, worker_func, &threadArgs[i]) != 0)
            ERR("Cannot create a thread");
    }

    explore_directory(startPath, buffer, &totalFiles);

    while (true) {
        pthread_mutex_lock(&mxProcessed);
        if (processedFiles == totalFiles) {
            pthread_mutex_unlock(&mxProcessed);
            break;
        }
        pthread_mutex_unlock(&mxProcessed);
        usleep(100000);
        kill(getpid(), SIGUSR1);
    }

    pthread_mutex_lock(&mxQuitFlag);
    quitFlag = true;
    pthread_mutex_unlock(&mxQuitFlag);

    for (int i = 0; i < threadCount; i++) {
        if (pthread_join(threadArgs[i].tid, NULL) != 0)
            ERR("pthread join");
    }

    if (pthread_join(signalThread, NULL) != 0)
        ERR("pthread join signal thread");

    for (int i = 0; i < MUTEX_COUNT; i++) {
        if (pthread_mutex_destroy(&mutexes[i]) != 0)
            ERR("Mutex destroy");
    }

    pthread_mutex_destroy(&mxPrint);
    circular_buffer_deinit(buffer);
    pthread_mutex_destroy(&mxQuitFlag);
    pthread_mutex_destroy(&mxProcessed);
    free(threadArgs);

    return EXIT_SUCCESS;
}

void ReadArgs(int argc, char** argv, char *startPath, int *threadCount) {
    if (argc != 3)
        ERR("Invalid number of arguments");

    strncpy(startPath, argv[1], strlen(argv[1]) + 1);
    startPath[PATH_MAX - 1] = '\0';

    *threadCount = atoi(argv[2]);
    if (*threadCount < 1)
        ERR("Invalid thread count");
}

void explore_directory(const char* dir_path, circular_buffer *buffer, int *totalFiles) {
    DIR *dir = opendir(dir_path);
    if (!dir)
        ERR("opendir");

    struct dirent *entry;
    struct stat statbuf;
    char path[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (snprintf(path, PATH_MAX, "%s/%s", dir_path, entry->d_name) >= PATH_MAX)
            ERR("Path length exceeds PATH_MAX");

        if (lstat(path, &statbuf) == -1)
            ERR("lstat");

        if (S_ISDIR(statbuf.st_mode)) {
            explore_directory(path, buffer, totalFiles);
        } else if (S_ISREG(statbuf.st_mode) && strstr(entry->d_name, ".txt")) {
            circular_buffer_enqueue(buffer, strdup(path));
            (*totalFiles)++;
        }
    }

    if (closedir(dir) == -1)
        ERR("closedir");
}

void* worker_func(void* voidArgs) {
    worker_args_t* args = voidArgs;
    char* file_path;

    for (;;) {
        pthread_mutex_lock(args->mxQuitFlag);
        bool quit = *(args->quitFlag);
        pthread_mutex_unlock(args->mxQuitFlag);

        if (quit && args->buffer->count == 0)
            break;

        file_path = circular_buffer_dequeue(args->buffer);
        if (file_path != NULL) {
            process_file(file_path, args->alphabetCounter, args->mutexes, args->worker_id, args->mxPrint);
            free(file_path);

            pthread_mutex_lock(args->mxProcessed);
            (*args->processedFiles)++;
            pthread_mutex_unlock(args->mxProcessed);
        } else {
            usleep(5000);
        }
    }

    return NULL;
}

void process_file(const char* file_path, int *alphabetCounter, pthread_mutex_t *mutexes, int worker_id, pthread_mutex_t *mxPrint) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
        ERR("Cannot open file");

    int localCounter[52] = {0};
    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytesRead; i++) {
            char c = buffer[i];
            if (isalpha(c)) {
                int index = isupper(c) ? c - 'A' : c - 'a' + 26;
                localCounter[index]++;
            }
        }
    }

    if (bytesRead == -1)
        ERR("read");

    close(fd);

    for (int i = 0; i < 52; i++) {
        if (localCounter[i] > 0) {
            pthread_mutex_lock(&mutexes[i]);
            alphabetCounter[i] += localCounter[i];
            pthread_mutex_unlock(&mutexes[i]);
        }
    }

    pthread_mutex_lock(mxPrint);
    printf("Pracownik nr %d zakończył zliczanie liter w pliku %s\n", worker_id, file_path);
    pthread_mutex_unlock(mxPrint);
}

void* signal_handler_thread(void* voidArgs) {
    signal_handler_args_t *args = voidArgs;
    sigset_t *mask = args->mask;

    while (true) {
        int sig;
        if (sigwait(mask, &sig) != 0)
            ERR("sigwait");

        if (sig == SIGUSR1) {
            print_alphabet_counters(args->alphabetCounter, args->mutexes);
        } else if (sig == SIGINT) {
            pthread_mutex_lock(args->mxQuitFlag);
            *(args->quitFlag) = true;
            pthread_mutex_unlock(args->mxQuitFlag);
            break;
        }
    }

    return NULL;
}

void print_alphabet_counters(int *alphabetCounter, pthread_mutex_t *mutexes) {
    printf("Wyniki:\n");
    for (int i = 0; i < 26; i++) {
        pthread_mutex_lock(&mutexes[i]);
        if (alphabetCounter[i] != 0)
            printf("%c=%d ", 'A' + i, alphabetCounter[i]);
        pthread_mutex_unlock(&mutexes[i]);
    }
    printf("\n");
    for (int i = 26; i < 52; i++) {
        pthread_mutex_lock(&mutexes[i]);
        if (alphabetCounter[i] != 0)
            printf("%c=%d ", 'a' + (i - 26), alphabetCounter[i]);
        pthread_mutex_unlock(&mutexes[i]);
    }
    printf("\n\n");
}
