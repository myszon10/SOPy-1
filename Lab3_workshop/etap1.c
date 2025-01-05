#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "circular_buffer.h"

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 2
#define ITEMS_TO_PRODUCE 10

typedef struct worker_args
{
    circular_buffer *buffer;
    int thread_id;
} worker_args_t;

void* producerFunc(void* voidArgs);
void* consumerFunc(void* voidArgs);

int main(int argc, char **argv) 
{
    bool *neededForOtherEtap = NULL;
    circular_buffer *buffer = circular_buffer_init(neededForOtherEtap);
    if(buffer == NULL)
        ERR("Failed to initialize buffer");

    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    worker_args_t producer_args[NUM_PRODUCERS];
    worker_args_t consumer_args[NUM_CONSUMERS];

    // Tworzenie wątków producentów
    for(int i = 0; i < NUM_PRODUCERS; i++)
    {
        producer_args[i].buffer = buffer;
        producer_args[i].thread_id = i+1;
        if(pthread_create(&producers[i], NULL, producerFunc, &producer_args[i]) != 0)
            ERR("Failed to create producer thread");
    }

    // Tworzenie wątków konsumentów
    for(int i = 0; i < NUM_CONSUMERS; i++)
    {
        consumer_args[i].buffer = buffer;
        consumer_args[i].thread_id = i+1;
        if(pthread_create(&consumers[i], NULL, consumerFunc, &consumer_args[i]) != 0)
            ERR("Failed to create consumer thread");
    }

    // Dołączanie wątków producentów
    for(int i = 0; i < NUM_PRODUCERS; i++)
    {
        if(pthread_join(producers[i], NULL) != 0)
            ERR("Failed to join producer thread");
    }

    // Dołączanie wątków konsumentów
    for(int i = 0; i < NUM_CONSUMERS; i++)
    {
        if(pthread_join(consumers[i], NULL) != 0)
            ERR("Failed to join consumer thread");
    }

    circular_buffer_deinit(buffer);
    printf("Test completed successfully.\n");
    return EXIT_SUCCESS;
}

void* producerFunc(void *voidArgs)
{
    worker_args_t *args = voidArgs;
    for(int i = 0; i < ITEMS_TO_PRODUCE; i++)
    {
        char *item = malloc(20 * sizeof(char));
        if(item == NULL)
            ERR("malloc");
        sprintf(item, "Item %d by P%d", i, args->thread_id);
        printf("Producer %d: Enqueuing '%s'\n", args->thread_id, item);
        circular_buffer_enqueue(args->buffer, item);
    }
    return NULL;
}

void* consumerFunc(void *voidArgs)
{
    worker_args_t *args = voidArgs;
    for(int i = 0; i < ITEMS_TO_PRODUCE; i++)
    {
        char *item = circular_buffer_dequeue(args->buffer);
        printf("Consumer %d: Dequeued '%s'\n", args->thread_id, item);
        free(item);
    }
    return NULL;
}
