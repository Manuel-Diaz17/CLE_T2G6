/**
 *  \file main.c (implementation file)
 *
 *  \brief Problem name: Read integers from one or several binary files and sort them by usninng bitonic sort algorithm and by making use of the MPI library.
 *
 *  \author Tiago Santos and Manuel Diaz - March 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#include "help_func.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    // Get rank and size
    int size, rank; 
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Initiate timer
    

    if (argc < 3 || strcmp(argv[1], "-f") != 0)
    {
        if (rank == 0)
        {
            fprintf(stderr, "[ERROR] Usage: %s -f <file1> ...\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Save the names of the files in an array
    int nFiles = argc - 2;
    char **names_files = (char **)malloc(nFiles * sizeof(char *));

    // Iterate for each file using the names_files array
    for (int i = 0; i < nFiles; i++)
    {
        ;
    }


}