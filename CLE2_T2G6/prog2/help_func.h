/**
 *  \file help_func.c (implementation file)
 *
 *  \brief Problem name: Read integers from one or several binary files and sort them by usninng bitonic sort algorithm and by making use of the MPI library.
 *
 *  This file has only the objective of having helpfull functions that are used in the main.
 *
 *  List of functionns created:
 *     \li validate_array;
 *     \li merge_subarrays;
 *     \li merge_sort;
 *
 *  \author Tiago Santos and Mannuel Diaz - March 2024
 */

#ifndef HELP_FUNC_H
# define HELP_FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * \brief Validate if array is sorted correctly. 
*/
int validate_array(int *array, int size);

/**
 * \brief Mergethe sub arrays in the sequence. 
*/
void merge_subarrays(int *array, int dir, int index, int n_to_sort);

/**
 * \brief Validate if array is sorted correctly. 
*/
void merge_sort(int *array, int size_sub, int index, int dir);

#endif