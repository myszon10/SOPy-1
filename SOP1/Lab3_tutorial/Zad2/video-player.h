#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)             \
    (__extension__({                               \
        long int __result;                         \
        do                                         \
            __result = (long int)(expression);     \
        while (__result == -1L && errno == EINTR); \
        __result;                                  \
    }))

#endif

#define ELAPSED(start, end) (((end).tv_sec - (start).tv_sec) * 1000000000L + (((end).tv_nsec - (start).tv_nsec)))

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define UNUSED(x) (void)(x)

#define FRAME_DATA_SIZE 64

#define BUFFER_SIZE 16

typedef struct video_frame
{
    int idx;
    char data[FRAME_DATA_SIZE];
} video_frame;

void random_sleep(int base, int add_time)
{
    struct timespec sleep_time = {0, (base + rand() % add_time) * 1000000L};
    TEMP_FAILURE_RETRY(nanosleep(&sleep_time, &sleep_time));
}

video_frame* decode_frame()
{
    static int frame_idx = 0;
    video_frame* frame = malloc(sizeof(video_frame));
    if (frame == NULL)
    {
        ERR("malloc");
    }
    frame->idx = frame_idx++;

    for (int i = 0; i < FRAME_DATA_SIZE - 1; i++)
    {
        frame->data[i] = 'a' + rand() % 26;
    }
    frame->data[FRAME_DATA_SIZE - 1] = 0;
    random_sleep(10, 30);
    return frame;
}

void transform_frame(video_frame* frame)
{
    for (int i = 0; i < FRAME_DATA_SIZE - 1; i++)
    {
        frame->data[i] = toupper(frame->data[i]);
    }
    random_sleep(10, 20);
}

void display_frame(video_frame* frame)
{
    static struct timespec last_frame = {0, 0};
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now))
        ERR("Failed to retrieve time!");
    time_t time_diff = ELAPSED(last_frame, now);
    printf("[frame %4d], %64s %ldus\n", frame->idx, frame->data, time_diff / 1000L);
    last_frame = now;
    free(frame);
    random_sleep(5, 5);
}
