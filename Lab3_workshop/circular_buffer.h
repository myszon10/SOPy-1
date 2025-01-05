#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 16 // Example size for the buffer

typedef struct circular_buffer {
    char *buffer[BUFFER_SIZE]; // Array of strings (file paths)
    int head;                  // Points to the next free slot for enqueue
    int tail;                  // Points to the next available item for dequeue
    int count;                 // Number of items currently in the buffer
    pthread_mutex_t mxBuffer;  // Mutex for synchronizing buffer access
    bool *quitFlag; 
} circular_buffer;

/**
 * Creates and initializes a circular buffer.
 * Returns a pointer to the buffer, or NULL on failure.
 */
circular_buffer* circular_buffer_init(bool *quitFlag);

/**
 * Destroys the circular buffer and frees all associated resources.
 */
void circular_buffer_deinit(circular_buffer *bufferArgs);

/**
 * Enqueues a string into the circular buffer.
 * Blocks if the buffer is full.
 */
void circular_buffer_enqueue(circular_buffer *bufferArgs, char *item);

/**
 * Dequeues a string from the circular buffer.
 * Blocks if the buffer is empty.
 * Returns a dynamically allocated string (must be freed by the caller).
 */
char* circular_buffer_dequeue(circular_buffer *bufferArgs);

#endif // CIRCULAR_BUFFER_H
