#include "video-player.h"
#include <pthread.h>

// TODO in stage 3
typedef struct circular_buffer
{
} circular_buffer;

void* decoder_func(void *voidArgs);

// circular_buffer* circular_buffer_create() {}
// void circular_buffer_push(circular_buffer* buffer, video_frame* frame) {}
// video_frame* circular_buffer_pop(circular_buffer* buffer) {}
// void circular_buffer_destroy(circular_buffer* buffer) {}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    video_frame *decoded_frame;
    video_frame *transform_frame;
    pthread_mutex_t mxDecoded = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mxTransfomed = PTHREAD_MUTEX_INITIALIZER;

    exit(EXIT_SUCCESS);
}

void* decoder_func(void *voidArgs)
{
    
}