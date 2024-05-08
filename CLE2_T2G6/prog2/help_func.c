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



/**
 * \brief Validate if array is sorted correctly. 
*/
int validate_array(int *array, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (array[i] > array[i + 1]) {
            return 0;
        }
    }
    return 1;
}

/**
 * \brief Merge the sub arrays in the sequence. 
*/
void merge_subarrays(int *array, int n_to_merge, int index, int dir) {
    if (n_to_merge > 1) {
        int k = n_to_merge / 2;
        for (int i = index; i < index + k; i++) {
            if (dir == (array[i] > array[i + k])) {
                int temp = array[i];
                array[i] = array[i + k];
                array[i + k] = temp;
            }
        }
        merge_subarrays(array, k, index, dir);
        merge_subarrays(array, k, index + k, dir);
    }
}

/**
 * \brief Recursively sorts a bitonic sequence.
*/
void merge_sort(int *array, int size_sub_array, int index, int dir) {
    if (size_sub_array > 1) {
        int k = size_sub_array / 2;
        merge_sort(array, k, index, 1);
        merge_sort(array, k, index + k, 0);
        merge_subarrays(array, size_sub_array, index, dir);
    }
}