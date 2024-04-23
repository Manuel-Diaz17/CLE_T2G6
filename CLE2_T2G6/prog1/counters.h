#ifndef COUNTERS_H
#define COUNTERS_H

/** \brief struct to store the counters of a file*/
struct FileCounters {
   char* file_name;        /* file name */  
   int total_num_of_words;    /* Number of total words */
   int total_words_with_two_equal_consonants;    /* Number of words with at least two equal consonants */
} FileCounters;

/**
 *  \brief Save the file names and initialize counters.
 *
 *  Operation carried out by the dispatcher.
 *
 *  \param n_file_names number of files
 *  \param file_names array with file names
 *
 *  \return value
 */
extern void storeFileNames(int n_file_names, char *file_names[]);

/**
 *  \brief update file counter with new results.
 *
 *  Operation carried out by the dispatcher.
 *
 *  \param file_id file identifier
 *  \param total_words number of total words
 *  \param total_words_with_two_equal_consonants number of total words with at least two equal consonants
 *
 *  \return value
 */
extern void saveResults (int file_id, int total_words, int total_words_with_two_equal_consonants);

/**
 *  \brief Print final results
 *
 *  Operation carried out by the dispatcher.
 *
 */
extern void printResults ();

#endif /* COUNTERS_H */