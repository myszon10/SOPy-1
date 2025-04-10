#include "channel.h"
#include "macros.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

channel_t* channel_open(const char* path) {

    // Use sem_name to initialize semaphore accompanied by channel
    char sem_name[PATH_MAX] = "sem_";
    strncpy(sem_name + 4, path, PATH_MAX - 5);
    sem_name[PATH_MAX-1] = 0;

    sem_t* sem = sem_open(sem_name, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        ERR("sem_open");
    }
    if (sem_wait(sem) == -1) {
        ERR("sem_wait");
    }
    int shm_fd;
    if ((shm_fd = shm_open(sem_name, O_RDWR, 0666)) == -1) {
        if ((shm_fd = shm_open(sem_name, O_CREAT | O_RDWR, 0666)) == -1) {
            ERR("shm_open");
        }
        if (ftruncate(shm_fd, sizeof(channel_t)) == -1) {
            ERR("ftruncate");
        }
    }
    channel_t* channel;
    if((channel = (channel_t*)mmap(NULL, sizeof(channel_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0))== MAP_FAILED) {
        ERR("mmap");
    }
    if (channel->status == CHANNEL_UNINITIALIZED) {
        channel->status = CHANNEL_EMPTY;
        channel->length = 0;
        channel->data_mtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        channel->producer_cv = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
        channel->consumer_cv = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    } else {
        // Channel already initialized, no need to reinitialize
    }

    if (sem_post(sem) == -1) {
        ERR("sem_post");
    }
    // Implement stage 1 here
    sem_close(sem);

    return channel;
}

void channel_close(channel_t* channel) {
    // Implement stage 1 here
    if (munmap(channel, sizeof(channel_t)) == -1) {
        ERR("munmap");
    }


}

int channel_produce(channel_t* channel, const char* produced_data, uint16_t length) {
    // Implement stage 3 here
    if (pthread_mutex_lock(&channel->data_mtx) != 0) {
        ERR("pthread_mutex_lock");
    }
    while (channel->status == CHANNEL_OCCUPIED) {
        if (pthread_cond_wait(&channel->producer_cv, &channel->data_mtx) != 0) {
            ERR("pthread_cond_wait");
        }
    }
    if (channel->status == CHANNEL_DEPLETED) {
        if (pthread_mutex_unlock(&channel->data_mtx) != 0) {
            ERR("pthread_mutex_unlock");
        }
        return -1;
    }
    if (length > CHANNEL_SIZE) {
        if (pthread_mutex_unlock(&channel->data_mtx) != 0) {
            ERR("pthread_mutex_unlock");
        }
        return -1;
    }
    memcpy(channel->data, produced_data, length);
    channel->length = length;
    channel->status = CHANNEL_OCCUPIED;
    if (pthread_cond_signal(&channel->consumer_cv) != 0) {
        ERR("pthread_cond_signal");
    }
    if (pthread_mutex_unlock(&channel->data_mtx) != 0) {
        ERR("pthread_mutex_unlock");
    }

    return 0;
}

int channel_consume(channel_t* channel, char* consumed_data, uint16_t* length) {
    // Implement stage 2 here
    if (pthread_mutex_lock(&channel->data_mtx) != 0) {
        ERR("pthread_mutex_lock");
    }
    while (channel->status == CHANNEL_EMPTY) {
        if (pthread_cond_wait(&channel->consumer_cv, &channel->data_mtx) != 0) {
            ERR("pthread_cond_wait");
        }
    }
    if (channel->status == CHANNEL_DEPLETED) {
        if (pthread_mutex_unlock(&channel->data_mtx) != 0) {
            ERR("pthread_mutex_unlock");
        }
        return -1;
    }

    if (channel->length > 0) {
        memcpy(consumed_data, channel->data, channel->length);
        *length = channel->length;
        channel->length = 0;
        channel->status = CHANNEL_EMPTY;
    } else {
        *length = 0;
    }
    write(STDOUT_FILENO, consumed_data, *length);
    if (pthread_cond_signal(&channel->producer_cv) != 0) {
        ERR("pthread_cond_signal");
    }
    if (pthread_mutex_unlock(&channel->data_mtx) != 0) {
        ERR("pthread_mutex_unlock");
    }


    return 0;
}

void duplicate_bytes(const char* input, uint16_t input_len, char* output, uint16_t* output_len) {
    for (uint16_t i = 0; i < input_len; ++i) {
        output[2 * i] = input[i];
        output[2 * i + 1] = input[i];
    }
    *output_len = input_len * 2;
}