#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "constants.h"

//struct used to store the counters of a file
struct FileCounters {
    char* file_name;
    int total_num_of_words;
    int total_words_with_two_equal_consonants;
};

//number of files
static int num_of_files;

//storage region for counters
static struct FileCounters * fmem;

//Save results and update the counters, performed by a worker thread
void saveResults(int file_id, int total_num_of_words, int total_words_with_two_equal_consonants) {

    fmem[file_id].total_num_of_words += total_num_of_words;
    fmem[file_id].total_words_with_two_equal_consonants += total_words_with_two_equal_consonants;

}

//Store file names, performed by the main thread
void storeFileNames(int n_file_names, char *file_names[]) {

    num_of_files = n_file_names;

    //memory allocation for the shared region storing and initializing counters to 0
    fmem = malloc(num_of_files * sizeof(struct FileCounters));

    for (int i=0; i<n_file_names; i++) {
        fmem[i].file_name = file_names[i];
        fmem[i].total_num_of_words = 0;
        fmem[i].total_words_with_two_equal_consonants = 0;
    }   

}

//Print the results, performed by the main thread
void printResults (){

    for (int i = 0; i<num_of_files; i++) {
        printf("\nFile name: %s\n", fmem[i].file_name);
        printf("Total number of words: %d\n", fmem[i].total_num_of_words);
        printf("Number of words with at least two equal consonants: %d\n", fmem[i].total_words_with_two_equal_consonants);
    }

}