#ifndef CHUNKS_H
#define CHUNKS_H

/**
 *  \brief Store a struct to inform that there are no more chunks to be processed.
 *
 *  Operation carried out by the main thread.
 *
 */
extern void endChunk();


/**
 *  \brief Store a chunk in the data transfer region.
 *
 *  Operation carried out by the main thread.
 *
 *  \param buffer pointer to the start of the chunk
 *  \param chunk_size number of bytes of the chunk
 *  \param file_id file identifier
 */
extern void putChunk (unsigned char * buffer, unsigned int chunk_size, unsigned int file_id);

/**
 *  \brief Get a chunk from the data transfer region.
 *
 *  Operation carried out by the workers.
 *
 *  \param worker_id consumer identification
 *
 *  \return value
 */
extern struct ChunkInfo getChunk (unsigned int worker_id);

/** \brief struct to store the information of one chunk*/
extern struct ChunkInfo {
   int file_id;        /* file identifier */  
   int chunk_size;    /* Number of bytes of the chunk */
   unsigned char * chunk_pointer;  /* Pointer to the start of the chunk */
} ChunkInfo;

#endif /* CHUNKS_H */