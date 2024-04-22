#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>

#include "chunks.h"
#include "constants.h"
#include "counters.h"
#include "countWordsFunctions.h"

//worker life cycle routine
static void *worker(void *par);

//process a chunk to count its words 
static void processChunk(struct ChunkInfo * chunk_info, int * total_num_of_words, int * total_words_with_two_equal_consonants);

//workers threads returns status array
int *status_workers;

//status of the main thread
int status_main_producer;

//number of bytes that a chunk should have
int num_bytes = N;  


int main(int argc, char *argv[]) {

    //char *locale = setlocale(LC_ALL, "");

    int *thread_status;

    //save filenames in the shared region and initialize counters to 0
    char *file_names[argc-2];
    for(int i=0; i<argc-2; i++) file_names[i] = argv[i+2];
    storeFileNames(argc-2, file_names);

    //measure time
    struct timespec start_time, finish_time;
    double elapsed_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

    //assign ids to each worker thread
    int num_of_threads = atoi(argv[1]);     //get the number of threads from the command line first argument
    status_workers = malloc(num_of_threads * sizeof(int));   //allocate memory to save the status of each worker
    pthread_t tIdWorkers[num_of_threads];
    unsigned int workers_id[num_of_threads];
    for (int i = 0; i < num_of_threads; i++)
        workers_id[i] = i;

    //generate worker threads
    for (int i = 0; i < num_of_threads; i++)
    if (pthread_create (&tIdWorkers[i], NULL, worker, &workers_id[i]) != 0)
    { 
        perror ("error on creating worker thread");
        exit (EXIT_FAILURE);
    }

    //generate the chunks of each file and put in FIFO
    for(int i=0;i<argc-2;i++){
        FILE * file_pointer;
        unsigned char byte;        //variable used to store each byte of the file  
        unsigned char *character;  //variable used to store the char

        file_pointer = fopen(file_names[i], "r");
        if (file_pointer == NULL) {
            printf("It occoured an error while openning file: %s \n", argv[i]);
            exit(EXIT_FAILURE);
        }

        //get file size
        fseek(file_pointer, 0, SEEK_END);
        int file_size = ftell(file_pointer);

        //seek file to the start
        fseek(file_pointer, 0, SEEK_SET);
        int bytes_processed = 0;
        int current_chunk_size;

        //while there are still bytes to create a chunk 
        while (bytes_processed < file_size) {

            int current_char_size = 0; //number of bytes read for the current char (it can be single byte or multibyte)

            //if it is the last chunk of the file
            if ( (bytes_processed + num_bytes) > file_size ) {
                //size of current chunk will be the remaining bytes
                current_chunk_size = file_size - bytes_processed;
            } else {
                current_chunk_size = num_bytes;  //chunk will have the default size of chunk
                fseek(file_pointer, bytes_processed + current_chunk_size, SEEK_SET);       // Seek file to the end of chunk

                //update the size of the chunk to ensure it doesn't cut a word or multibyte character
                while (true) {                  
                    byte = fgetc(file_pointer);

                    current_char_size = 1;  
                    character = malloc((1+1)* sizeof(unsigned char) );      //the last byte of the character is required to be 0
                    character[0] = byte;

                    //determine if the byte represents a 3-byte character or a single byte character
                    //safe-cut characters include whitespace (single byte), separation(single or multibyte), and punctuation(single or multibyte)
                    if ( byte > 224 && byte < 240) {     // 3-byte char
                        // Create the 3-byte character
                        character = realloc(character, (3+1)* sizeof(unsigned char) );      //reallocate memory to accomodate a 3-byte character
                        byte = fgetc(file_pointer);    //read another byte
                        character[1] = byte;
                        byte = fgetc(file_pointer);    //read another byte
                        character[2] = byte;
                        character[3] = 0;
                        current_char_size += 2;
                    } else {                        //it's a single byte char
                        character[1] = 0;
                    }

                    //if it is a safe place to cut the chunk, so we break
                    if (is_whitespace(character) || is_separation(character) || is_punctuation(character)) {
                        break;
                    }

                    //increment the size of chunk
                    current_chunk_size += current_char_size;
                }
            }

            //seek file to the initial of the chunk
            fseek(file_pointer, bytes_processed, SEEK_SET);

            //create buffer for the chunk with the current chunk + the last character
            unsigned char* buffer = malloc(current_chunk_size + current_char_size);
            int s = fread(buffer, current_chunk_size + current_char_size, 1, file_pointer);
            if (s != 1)
                printf("Error creating chunk buffer.");

            //save chunk in FIFO
            putChunk(buffer, current_chunk_size + current_char_size, i);

            bytes_processed += current_chunk_size;
        }

        //close file
        fclose(file_pointer);
    }

    //save a struct in fifo for each thread to know that there are no more chunks to process
    for (int i = 0; i < num_of_threads; i++) {
        endChunk();
    }

   //waiting for the termination of the intervening worker threads
    for (int i = 0; i < num_of_threads; i++)
    { 
        if (pthread_join (tIdWorkers[i], (void *) &thread_status) != 0)
        { 
            perror ("Error on waiting for thread worker");
            exit (EXIT_FAILURE);
        }
        printf ("Thread worker, with id %u, has terminated with the status %d\n", i, *thread_status);
    }

    //measure time
    clock_gettime(CLOCK_MONOTONIC_RAW, &finish_time);
    elapsed_time = (finish_time.tv_sec - start_time.tv_sec);
    elapsed_time += (finish_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

    //print final results
    printResults();
    printf("\nElapsed time = %.7f s\n", elapsed_time);
}

//its role is to get chunks of data and count the words. After that, it incrementes the counters in shared region.
static void *worker(void *par) {
    unsigned int id = *((unsigned int *) par);

    while (true) {
        //get chunk of data
        struct ChunkInfo chunk_info = getChunk(id);

        //checks if it is the chunk that tells that there are no more chunks to process
        if (chunk_info.file_id == -1) break;

        //process chunk of data
        int total_num_of_words = 0;
        int total_words_with_two_equal_consonants = 0;
        processChunk(&chunk_info, &total_num_of_words, &total_words_with_two_equal_consonants);

        //free the memory of the buffer
        free(chunk_info.chunk_pointer);

        //save chunk of data
        saveResults(id, chunk_info.file_id, total_num_of_words, total_words_with_two_equal_consonants);
    }

    status_workers[id] = EXIT_SUCCESS;
    pthread_exit (&status_workers[id]);
}

static void processChunk(struct ChunkInfo * chunk_info, int * total_num_of_words, int * total_words_with_two_equal_consonants) {
    //flag used to determine if the algorithm is handling the char inside a word context or not
    bool inword = false;  

    //flag used to check if the word have two equal consonants or not    
    bool has_two_equal_consonants = false;
    int consonant_count[26] = {0};

    unsigned char byte;             //variable used to store each byte of the chunk
    int i = 0;                      //counter to make sure to read only chunk_size bytes
    unsigned char *character;       //initialization of variable used to store the char (singlebyte or multibyte)

    while (i < (*chunk_info).chunk_size) {

        //construction of the character
        byte = (*chunk_info).chunk_pointer[i];        //read a byte
        character = malloc((1+1)* sizeof(unsigned char) );      //allocate memory to store the character. For now, it is a single byte
        character[0] = byte;

        if (byte > 192 && byte < 224) {   //if it is a 2-byte char
            i++;
            character = realloc(character, (2+1)* sizeof(unsigned char) );
            character[1] = (*chunk_info).chunk_pointer[i];
            character[2] = 0;
        } else if ( byte > 224 && byte < 240) {     //if it is a 3-byte char
            character = realloc(character, (3+1)* sizeof(unsigned char) );
            i++;
            character[1] = (*chunk_info).chunk_pointer[i];
            i++;
            character[2] = (*chunk_info).chunk_pointer[i];
            character[3] = 0;
        } else if ( byte > 240 ) {     //if it is a 4-byte char
            character = realloc(character, (4+1)* sizeof(unsigned char) );
            i++;
            character[1] = (*chunk_info).chunk_pointer[i];
            i++;
            character[2] = (*chunk_info).chunk_pointer[i];
            i++;
            character[3] = (*chunk_info).chunk_pointer[i];
            character[4] = 0;
        } else {        //if it is a single byte char
            character[1] = 0;
        }

        //convert multybyte chars to singlebyte chars according the Portuguese special characters encoding
        if (character[0] == 0xC3) {
            byte = convert_special_chars(character[1]);
            character = realloc(character, (1+1)* sizeof(unsigned char) );      //convert multibyte char to single byte
            character[0] = byte;
            character[1] = 0;
        }

        //strategy to count the words
        if (inword) {
            if (is_consonant(character)) {
                consonant_count[tolower(*character) - 'a']++;
            } else if (is_vowel(character) || is_decimal_digit(character) || is_underscore(character) || is_apostrophe(character)) {
                inword= true;
            } else if (is_whitespace(character) || is_separation(character) || is_punctuation(character)) {
                inword = false;
                has_two_equal_consonants = false;
                    for (int j = 0; j < 26; j++) {
                        if (consonant_count[j] >= 2) {
                            has_two_equal_consonants = true;
                            if (has_two_equal_consonants) {
                                *total_words_with_two_equal_consonants += 1;
                                break;
                            }
                        }
                    }    
                memset(consonant_count, 0, sizeof(consonant_count));
            }
        } else {
            if (is_whitespace(character) || is_separation(character) || is_punctuation(character) || is_apostrophe(character)) {
                inword = false;
            } else if (is_vowel(character)) {
                inword = true;
                *total_num_of_words += 1;
            } else if (is_consonant(character)) {
                inword = true;
                *total_num_of_words += 1;
                consonant_count[tolower(*character) - 'a']++;
            } else if (is_decimal_digit(character) || is_underscore(character)) {
                inword = true;
                *total_num_of_words += 1;
            }
        }

        i++;
        free(character);
    }
}