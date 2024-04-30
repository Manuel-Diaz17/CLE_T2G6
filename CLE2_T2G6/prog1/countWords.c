#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>
#include <pthread.h>
#include <mpi.h>

#include "constants.h"
#include "counters.h"
#include "countWordsFunctions.h"

//struct used to store the results of a file
struct FileResults {
   int file_id;
   int total_num_of_words;
   int total_words_with_two_equal_consonants;
};

//struct used to store the information of a chunk
struct ChunkInfo {
    int file_id;
    int chunk_size;
    unsigned char* chunk_info;
};

//dispatcher life cycle routine
static void dispatcher(char *file_names[], int num_of_files);

//worker life cycle routine
static void *worker(int rank);

//process a chunk to count its words 
static void processChunk(struct ChunkInfo * chunk_info, int * total_num_of_words, int * total_words_with_two_equal_consonants);

//number of workers
int num_of_workers;

//number of bytes that a chunk should have
int num_bytes = N;  


int main(int argc, char *argv[]) {

    int rank, size;

    //initialize MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &size);

    //setlocale(LC_ALL, "en_US.UTF-8");

    num_of_workers = size - 1;

    if(num_of_workers <= 0) {
        fprintf(stderr, "You must have at least 1 worker, meaning, n value must be higher than 1. \n"); 
        MPI_Finalize();
        return EXIT_FAILURE;
    } else {
        if (rank == 0) {

            //measure time
            struct timespec start_time, finish_time;
            double elapsed_time;
            clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

            //read file names
            char *file_names[argc-1];

            for (int i = 1; i<argc; i++) {
                file_names[i-1] = argv[i];
            }

            //launch dispatcher
            dispatcher(file_names, argc-1);

            //measure time
            clock_gettime(CLOCK_MONOTONIC_RAW, &finish_time);
            elapsed_time = (finish_time.tv_sec - start_time.tv_sec);
            elapsed_time += (finish_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

            //print final results
            printResults();
            printf("\nElapsed time = %.7f s\n", elapsed_time);
        } else {

            //launch worker
            worker(rank);
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}

static void dispatcher(char *file_names[], int num_of_files) {

    bool msgRec[num_of_workers];
    struct FileResults results[num_of_workers];

    for (int i = 0; i < num_of_workers; i++) {
        msgRec[i] = false;
    }

    //id of worker that will receive the next chunk to process
    int current_worker_id = 1;

    //number of chunks sent to workers
    int num_of_chunks_sent = 0;

    //initialize counters to 0 for each file
    storeFileNames(num_of_files, file_names);

    //generate the chunks of each file
    for(int i=0; i<num_of_files; i++){
        
        FILE * file_pointer;
        unsigned char byte;        //variable used to store each byte of the file  
        unsigned char *character;  //variable used to store the char

        //open file
        file_pointer = fopen(file_names[i], "r");
        if (file_pointer == NULL) {
            printf("It occoured an error while openning file: %s \n", file_names[i]);
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

            //array with chunk information
            unsigned char * chunk = (unsigned char*) malloc(current_chunk_size + current_char_size + sizeof(int));
            chunk[0] = i;
            int s = fread(chunk+1, current_chunk_size + current_char_size, 1, file_pointer);
            if (s != 1)
                printf("Error creating chunk buffer.");

            MPI_Send(chunk, current_chunk_size + current_char_size + sizeof(int), MPI_BYTE, current_worker_id, 1, MPI_COMM_WORLD);
            
            //update current_worker_id and num_of_chunks_sent variables
            current_worker_id = (current_worker_id % num_of_workers) + 1;
            num_of_chunks_sent += 1;

            bytes_processed += current_chunk_size;

            if (num_of_chunks_sent >= num_of_workers) {

                //check if results from workers are available
                for (int i = 1; i <= num_of_workers; i++) {

                    if (!msgRec[i-1]) {
                        MPI_Recv(&results[i-1], sizeof(struct FileResults), MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        //save results
                        saveResults(results[i-1].file_id, results[i-1].total_num_of_words, results[i-1].total_words_with_two_equal_consonants);

                        num_of_chunks_sent -= 1;
                        msgRec[i-1] = false;
                    }
                }
            }
        }

        //close file
        fclose(file_pointer);
    }

    //receive results of last chunks from workers
    while (num_of_chunks_sent > 0) {
        for (int i = 1; i <= num_of_workers; i++) {

            if (!msgRec[i-1]) {
                MPI_Recv(&results[i-1], sizeof(struct FileResults), MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //save results
                saveResults(results[i-1].file_id, results[i-1].total_num_of_words, results[i-1].total_words_with_two_equal_consonants);
            
                num_of_chunks_sent-=1;
                msgRec[i-1] = false;
            }
        }
    }

    
    //send message to each process to know that there are no more chunks to process
    for (int i = 1; i <= num_of_workers; i++) {
        unsigned char* last_chunk = malloc(sizeof(unsigned char));
        last_chunk[0] = 255;

        //send special chunk to Worker
        MPI_Send(last_chunk, sizeof(unsigned char), MPI_BYTE, i, 1, MPI_COMM_WORLD);
    }

}

//its role is to get chunks of data and count the words. After that, it incrementes the counters in shared region.
static void *worker(int rank) {

    while (true) {

        //struct to get chunk of data
        struct ChunkInfo new_chunk;

        //get message size
        MPI_Status status;
        MPI_Probe(0, 1, MPI_COMM_WORLD, &status);
        int message_size;
        MPI_Get_count(&status, MPI_BYTE, &message_size);

        //alocate memory to read the chunk information
        new_chunk.chunk_info = (unsigned char*) malloc(message_size);
        MPI_Recv(new_chunk.chunk_info, message_size, MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //checks if it is the chunk that tells that there are no more chunks to process
        if (new_chunk.chunk_info[0] == 255) break;

        //convert info to the struct ChunkInfo
        new_chunk.file_id = new_chunk.chunk_info[0];
        new_chunk.chunk_info = new_chunk.chunk_info + 1;
        new_chunk.chunk_size = message_size - sizeof(int);

        //process chunk of data
        int total_num_of_words = 0;
        int total_words_with_two_equal_consonants = 0;
        processChunk(&new_chunk, &total_num_of_words, &total_words_with_two_equal_consonants);

        //free the memory of the buffer
        free(new_chunk.chunk_info-1);

        //send results back to dispatcher
        struct FileResults results;
        results.file_id = new_chunk.file_id;
        results.total_num_of_words = total_num_of_words;
        results.total_words_with_two_equal_consonants = total_words_with_two_equal_consonants;
        
        MPI_Send(&results, sizeof(struct FileResults), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
    return 0;
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
        byte = (*chunk_info).chunk_info[i];        //read a byte
        character = malloc((1+1)* sizeof(unsigned char) );      //allocate memory to store the character. For now, it is a single byte
        character[0] = byte;

        if (byte > 192 && byte < 224) {   //if it is a 2-byte char
            i++;
            character = realloc(character, (2+1)* sizeof(unsigned char) );
            character[1] = (*chunk_info).chunk_info[i];
            character[2] = 0;
        } else if ( byte > 224 && byte < 240) {     //if it is a 3-byte char
            character = realloc(character, (3+1)* sizeof(unsigned char) );
            i++;
            character[1] = (*chunk_info).chunk_info[i];
            i++;
            character[2] = (*chunk_info).chunk_info[i];
            character[3] = 0;
        } else if ( byte > 240 ) {     //if it is a 4-byte char
            character = realloc(character, (4+1)* sizeof(unsigned char) );
            i++;
            character[1] = (*chunk_info).chunk_info[i];
            i++;
            character[2] = (*chunk_info).chunk_info[i];
            i++;
            character[3] = (*chunk_info).chunk_info[i];
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