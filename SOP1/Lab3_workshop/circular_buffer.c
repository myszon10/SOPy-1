#include "circular_buffer.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

circular_buffer* circular_buffer_init(bool *quitFlag) 
{
    circular_buffer *buffer = malloc(sizeof(circular_buffer));
    if(buffer == NULL)
        ERR("malloc");

    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->quitFlag = quitFlag;
    
    if(pthread_mutex_init(&buffer->mxBuffer, NULL) != 0)
    {
        free(buffer);
        ERR("pthread_mutex_init");
    }
    return buffer;
}

void circular_buffer_deinit(circular_buffer *bufferArgs) 
{
    if(bufferArgs == NULL)
        return;

    pthread_mutex_destroy(&bufferArgs->mxBuffer);
    free(bufferArgs);
}

void circular_buffer_enqueue(circular_buffer *bufferArgs, char *item) 
{
    if(bufferArgs == NULL)
        ERR("Nullptr passed as argument");

    while(bufferArgs->count == BUFFER_SIZE) // bufor jest pełny
    {
        // ERR("Cannot add to a full buffer");
        usleep(5000); // wait 5ms (busy waiting)
    }

    pthread_mutex_lock(&bufferArgs->mxBuffer);
    bufferArgs->buffer[bufferArgs->head] = item; // dodanie elementu na pozycji head 

    bufferArgs->head = (bufferArgs->head + 1) % BUFFER_SIZE; // cykliczne przesunięcie head

    bufferArgs->count++; // zwiększenie licznika elementów
    pthread_mutex_unlock(&bufferArgs->mxBuffer);
}

char* circular_buffer_dequeue(circular_buffer *bufferArgs) 
{
    while (true) {
        pthread_mutex_lock(&bufferArgs->mxBuffer);

        // Sprawdź, czy bufor jest pusty
        if (bufferArgs->count > 0) {
            char *item = bufferArgs->buffer[bufferArgs->tail]; // wydobycie elementu

            bufferArgs->tail = (bufferArgs->tail + 1) % BUFFER_SIZE; // przesunięcie wskażnika tail

            bufferArgs->count--; // zmniejszenie liczby elementów
            pthread_mutex_unlock(&bufferArgs->mxBuffer);
            return item;
        }

        // Jeśli bufor jest pusty, sprawdź quitFlag
        if (*(bufferArgs->quitFlag)) {
            pthread_mutex_unlock(&bufferArgs->mxBuffer);
            return NULL;
        }

        pthread_mutex_unlock(&bufferArgs->mxBuffer);
        usleep(5000); // Czekaj chwilę przed ponowną próbą
    }
}
