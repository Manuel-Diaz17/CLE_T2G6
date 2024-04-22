#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "constants.h"

//struct used to store the info of one chunk
struct ChunkInfo {
   int file_id;
   int chunk_size;
   unsigned char * chunk_pointer;
};

//status of the main thread
extern int status_main_producer;

//workers threads returns status array
extern int *status_workers;

//storage region for chunks
static struct ChunkInfo cmem[K];

static unsigned int insertion_pointer;

static unsigned int retrieval_pointer;

//flag to check if the transfer region is full
static bool transfer_region_full;

//locking flag which warrants mutual exclusion inside the monitor
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

//flag which warrants that the data transfer region is initialized exactly once
static pthread_once_t init = PTHREAD_ONCE_INIT;;

//main synchronization point when the data transfer region is full
static pthread_cond_t fifo_full;

//workers synchronization point when the data transfer region is empty
static pthread_cond_t fifo_empty;

//Initialization of the data transfer region, performed by the monitor
static void initialization (void)
{
    insertion_pointer = 0;
    retrieval_pointer = 0;
    transfer_region_full = false;

    pthread_cond_init (&fifo_full, NULL);
    pthread_cond_init (&fifo_empty, NULL);
}

//Store a struct in fifo to inform that there are no more chunks to be processed, performed by the main thread
void endChunk() {
    //entering monitor
    if ((status_main_producer = pthread_mutex_lock (&accessCR)) != 0)
        { 
            errno = status_main_producer;
            perror ("error on entering monitor(CF)");
            status_main_producer = EXIT_FAILURE;
            pthread_exit (&status_main_producer);
        }

    //wait if the transfer region is full
    while (transfer_region_full)
    { if ((status_main_producer = pthread_cond_wait (&fifo_full, &accessCR)) != 0)
        { errno = status_main_producer;
            perror ("error on waiting in fifoFull");
            status_main_producer = EXIT_FAILURE;
            pthread_exit (&status_main_producer);
        }
    }

    //store values in the FIFO
    cmem[insertion_pointer].file_id = -1;
    cmem[insertion_pointer].chunk_size = -1;
    cmem[insertion_pointer].chunk_pointer =  NULL;
    insertion_pointer = (insertion_pointer + 1) % K;
    transfer_region_full = (insertion_pointer == retrieval_pointer);

    // let a worker know that a value has been stored    
    if ((status_main_producer = pthread_cond_signal (&fifo_empty)) != 0)
    {
        errno = status_main_producer;
        perror ("error on signaling in fifoEmpty");
        status_main_producer = EXIT_FAILURE;
        pthread_exit (&status_main_producer);
    }

    //exiting monitor
    if ((status_main_producer = pthread_mutex_unlock (&accessCR)) != 0)
    {   
        errno = status_main_producer;
        perror ("error on exiting monitor(CF)");
        status_main_producer = EXIT_FAILURE;
        pthread_exit (&status_main_producer);
    }
}

//Store a chunk in the data transfer region, performed by the main thread 
void putChunk (unsigned char * buffer, unsigned int chunk_size, unsigned int file_id)
{
    //entering monitor
    if ((status_main_producer = pthread_mutex_lock (&accessCR)) != 0)
    {   
        errno = status_main_producer;
        perror ("error on entering monitor(CF)");
        status_main_producer = EXIT_FAILURE;
        pthread_exit (&status_main_producer);
    }
    pthread_once (&init, initialization);

    //wait if the data transfer region is full
    while (transfer_region_full)
    { if ((status_main_producer = pthread_cond_wait (&fifo_full, &accessCR)) != 0)
        { 
            errno = status_main_producer;
            perror ("error on waiting in fifoFull");
            status_main_producer = EXIT_FAILURE;
            pthread_exit (&status_main_producer);
        }
    }

    //store values in the FIFO
    cmem[insertion_pointer].file_id = file_id;
    cmem[insertion_pointer].chunk_size = chunk_size;
    cmem[insertion_pointer].chunk_pointer =  buffer;
    insertion_pointer = (insertion_pointer + 1) % K;
    transfer_region_full = (insertion_pointer == retrieval_pointer);

    //let a worker know that a value has been stored
    if ((status_main_producer = pthread_cond_signal (&fifo_empty)) != 0)
    { 
        errno = status_main_producer;
        perror ("error on signaling in fifoEmpty");
        status_main_producer = EXIT_FAILURE;
        pthread_exit (&status_main_producer);
    }

    //exiting monitor
    if ((status_main_producer = pthread_mutex_unlock (&accessCR)) != 0)
    { 
        errno = status_main_producer;
        perror ("error on exiting monitor(CF)");
        status_main_producer = EXIT_FAILURE;
        pthread_exit (&status_main_producer);
    }
}

//Get a chunk from the data transfer region, performed by the workers
struct ChunkInfo getChunk (unsigned int worker_id)
{
    struct ChunkInfo chunk_info;

    //entering monitor
    if ((status_workers[worker_id] = pthread_mutex_lock (&accessCR)) != 0)
    {   
        errno = status_workers[worker_id];
        perror ("error on entering monitor(CF)");
        status_workers[worker_id] = EXIT_FAILURE;
        pthread_exit (&status_workers[worker_id]);
    }

    pthread_once (&init, initialization);

    //wait if the data transfer region is empty
    while ((insertion_pointer == retrieval_pointer) && !transfer_region_full)
    { 
        if ((status_workers[worker_id] = pthread_cond_wait (&fifo_empty, &accessCR)) != 0)
        { 
            errno = status_workers[worker_id];
            perror ("error on waiting in fifoEmpty");
            status_workers[worker_id] = EXIT_FAILURE;
            pthread_exit (&status_workers[worker_id]);
        }
    }

    //retrieve a  value from the FIFO
    chunk_info = cmem[retrieval_pointer];
    retrieval_pointer = (retrieval_pointer + 1) % K;
    transfer_region_full = false;

    //let a producer know that a value has been retrieved
    if ((status_workers[worker_id] = pthread_cond_signal (&fifo_full)) != 0)       
    {   
        errno = status_workers[worker_id];
        perror ("error on signaling in fifoFull");
        status_workers[worker_id] = EXIT_FAILURE;
        pthread_exit (&status_workers[worker_id]);
    }

    //exiting monitor
    if ((status_workers[worker_id] = pthread_mutex_unlock (&accessCR)) != 0)
    { 
        errno = status_workers[worker_id];
        perror ("error on exiting monitor(CF)");
        status_workers[worker_id] = EXIT_FAILURE;
        pthread_exit (&status_workers[worker_id]);
    }

    return chunk_info;
}