#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <stdint.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t exit_flag = 0;

void sigint_handler(int signum) {
    exit_flag = 1;
}

typedef struct {
    int counters;
    pthread_mutex_t mutex;
    int hit_points;
    int total_randomized_points;
    int a;
    int b;
} SharedMemory;

double func(double x) {
    usleep(2000);
    return exp(-x * x);
}

int randomize_points(int N, float a, float b) {
    int i = 0;
    for (; i < N; ++i) {
        double rand_x = ((double)rand() / RAND_MAX) * (b - a) + a;
        double rand_y = ((double)rand() / RAND_MAX);
        double real_y = func(rand_x);

        if (rand_y <= real_y)
            i++;
    }
    return i;
}

double summarize_calculations(uint64_t total_randomized_points, uint64_t hit_points, float a, float b) {
    return (b - a) * ((double)hit_points / (double)total_randomized_points);
}

int safe_lock(pthread_mutex_t* mtx, SharedMemory *shm) {
    int ret = pthread_mutex_lock(mtx);
    if (ret == 0) {
        return 0;  // Successfully locked
    }

    printf("Mutex lock failed: %d == %d\n", EOWNERDEAD, ret);
    if (ret == EOWNERDEAD) {

        ret = pthread_mutex_consistent(mtx);
        if (ret != 0) {
            ERR("pthread_mutex_consistent failed");
        }
        pthread_mutex_unlock(mtx);

        // Decrement the counter for the process that died
        pthread_mutex_lock(mtx);

        shm->counters--;
        printf("Process died, remaining processes: %d\n", shm->counters);

        return 0;
    }

    return ret;
}

int random_death_lock(pthread_mutex_t* mtx, SharedMemory *shm) {
    int ret = safe_lock(mtx, shm);
    if (ret) return ret;

    // 2% chance to die
    if (rand() % 4 == 0) abort();
    return ret;
}

void usage(char* argv[]) {
    printf("%s a b N - calculating integral with multiple processes\n", argv[0]);
    printf("a - Start of segment for integral (default: -1)\n");
    printf("b - End of segment for integral (default: 1)\n");
    printf("N - Size of batch to calculate before reporting to shared memory (default: 1000)\n");
}

void cleanup_shared_memory(SharedMemory* shm) {
    safe_lock(&shm->mutex, shm);
    shm->counters--;
    if (shm->counters  == 0) {
        shm_unlink("/count");
    }
    pthread_mutex_unlock(&shm->mutex);
    pthread_mutex_destroy(&shm->mutex);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, sigint_handler);

    usage(argv);

    int a = -1;
    int b = 1;
    int N = 1000;

    if (argc >= 2) a = atoi(argv[1]);
    if (argc >= 3) b = atoi(argv[2]);
    if (argc >= 4) N = atoi(argv[3]);

    int shm_fd;
    char shm_name[32];
    sprintf(shm_name, "/count");
    unlink(shm_name);
    char sem_name[32];
    sprintf(sem_name, "/sem");
    sem_t *sem = sem_open(sem_name, O_CREAT, 0666, 1);

    if (sem == SEM_FAILED) {
        ERR("sem_open");
    }
     sem_post(sem);

    if (sem_wait(sem) == -1) {
        ERR("sem_wait");
    }

    if ((shm_fd = shm_open(shm_name, O_RDWR, 0666)) == -1) {
        if ((shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666)) == -1) {
            ERR("shm_open");
        }
        if (ftruncate(shm_fd, sizeof(SharedMemory)) == -1) {
            ERR("ftruncate");
        }
    }

    SharedMemory *shm;

    if ((shm = (SharedMemory*)mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        ERR("mmap");

    if (shm->counters == 0) {
        shm->counters = 1;
        shm->hit_points = 0;
        shm->total_randomized_points = 0;
        shm->a = a;
        shm->b = b;

        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST);

        if (pthread_mutex_init(&shm->mutex, &mutex_attr) != 0) {
            ERR("pthread_mutex_init");
        }

        pthread_mutexattr_destroy(&mutex_attr);
    } else {
        safe_lock(&shm->mutex,shm);
        shm->counters++;
        pthread_mutex_unlock(&shm->mutex);
    }

    if (sem_post(sem) == -1) {
        ERR("sem_post");
    }
    sem_close(sem);

    printf("Currently collaborating processes: %d\n", shm->counters);

    while (exit_flag != 1) {
        int hit_points = randomize_points(N, a, b);
        //safe_lock(&shm->mutex, shm);
        random_death_lock(&shm->mutex, shm);
        // Lock the mutex and handle possible process death during the lock



        shm->hit_points += hit_points;
        shm->total_randomized_points += N;
        pthread_mutex_unlock(&shm->mutex);

        if (exit_flag == 1) {
            break;
        }
    }

    safe_lock(&shm->mutex, shm);
    double result = summarize_calculations(shm->total_randomized_points, shm->hit_points, a, b);
    sleep(2);  // Allow some time for the last calculations to stabilize
    printf("%d processes finished\n", shm->counters);
    if (shm->counters == 1) {
        printf("Result: %f\n", result);
    }
    pthread_mutex_unlock(&shm->mutex);

    cleanup_shared_memory(shm);
    //munmap(shm, sizeof(SharedMemory));

    return EXIT_SUCCESS;
}