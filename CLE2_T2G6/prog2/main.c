/**
 *  \file main.c (implementation file)
 *
 *  \brief Problem name: Read integers from one or several binary files and sort them by usninng bitonic sort algorithm and by making use of the MPI library.
 *
 *  \author Tiago Santos and Manuel Diaz - March 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <linux/time.h>

#include "help_func.h"


void dispatcher(char ***fileNames, int fileAmount);
static double get_delta_time(void);

/**
 *  \brief Main function.
 *
 */
int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    // Get rank and size
    int size, rank; 
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    

    if (argc < 3 || strcmp(argv[1], "-f") != 0)
    {
        if (rank == 0)
        {
            fprintf(stderr, "ERROR! Usage: mpiexec -n [number of processors] ./%s -f -f <file1> ...\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Number of files
    int nFiles = argc - 2;


    // Iterate for each file annd sort them
    for (int i = 0; i < nFiles; i++)
    {
        FILE *file;
        int total_n = 0; // Total number of integers in the file

        // Make Distributor open the file and read the number of integers
        if (rank == 0)
        {
            file = fopen(argv[i + 2], "rb");
            if (!file)
            {
                fprintf(stderr, "ERROR! Error opening file: %s\n", argv[i + 2]);
                continue;
            }
            //start timer
            (void) get_delta_time();
            
            printf("Current file being processed: %s\n", argv[i + 2]);
            // Read the number of integers in the file
            size_t read_result = fread(&total_n, sizeof(int), 1, file);
            if (read_result != 1) {
                fprintf(stderr, "ERROR! Failed to read the number of integers from file: %s\n", argv[i + 2]);
                fclose(file);
                continue;
            }
        }

        // Broadcast the number of integers to all processors
        MPI_Bcast(&total_n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Allocate memory for the integers
        int *array = (int *)malloc(total_n * sizeof(int));


        // Distributor reads all the integers in the file and stores them in the array
        if (rank == 0)
        {
            size_t read_result = fread(array, sizeof(int), total_n, file);
            if (read_result != 1) {
                fprintf(stderr, "ERROR! Failed to read the number of integers from file: %s\n", argv[i + 2]);
                fclose(file);
                continue;
            }
            fclose(file);
        }

        // Broadcast the integers to all processors
        MPI_Bcast(array, total_n, MPI_INT, 0, MPI_COMM_WORLD);

        // Allocate memory for the local array
        int *local_array = (int *)malloc(total_n * sizeof(int));
        
        //Define chunk size in equal parts for each processor
        int chunk_size = total_n / size;

        // Scatter the integers inn chunks to all processors
        MPI_Scatter(array, chunk_size, MPI_INT, array, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);        

        // Use bitonic sort algorithm to sort the integers first array goes in ascending order
        merge_sort(local_array, chunk_size, 0, 1);

        // Iterate for each processor
        for (int j = 1; j <= size; j *= 2){

            // Define the partner processor for the current processor to proceed with the iteration
            int partner = rank ^ j;

            MPI_Sendrecv_replace(array, chunk_size, MPI_INT, partner, 0, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Merge the integers in the array
            if (rank & j){
                merge_subarrays(array, chunk_size, 0, 0);
            }
            else
            {
                merge_subarrays(array, chunk_size, 0, 1);
            }
        }

        MPI_Gather(array, chunk_size, MPI_INT, array, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

        // Distributor merge sorts all the local arrays
        if (rank == 0)
        {
            merge_sort(array, total_n, 0, 1);
        }

        // Validate if the array is sorted correctly
        if (rank == 0)
        {
            if (validate_array(array, total_n))
            {
                printf("Correctly sorted.\n");
            } 
            else 
            {
                printf("Not correctly sorted.\n");
            }
        }

        free(array);
        free(local_array);
    }

    // End timer
    if (rank == 0)
    {
        // Print the execution time
        printf("Execution time: %f\n", get_delta_time());
        
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}

/**
 *  \brief Get the process time that has elapsed since last call of this time.
 *
 *  \return process elapsed time
 */
static double get_delta_time(void)
{
	static struct timespec t0, t1;

	t0 = t1;
	if(clock_gettime (CLOCK_MONOTONIC, &t1) != 0)
	{
		perror ("clock_gettime");
		exit(1);
	}
	return (double) (t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double) (t1.tv_nsec - t0.tv_nsec);
}
