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
 * \brief Mergethe sub arrays in the sequence. 
*/
void merge_subarrays(int *array, int dir, int index, int n_to_sort){

    if (index > 1){
        
        int k = index / 2;

        for (int i = 0; i < n_to_sort; i++){
            if (dir == (array[i] > array[i + k])){
                int temp = array[i];
                array[i] = array[i + k];
                array[i + k] = temp;
            }
        }
        merge_subarrays(array, dir, k, n_to_sort);
        merge_subarrays(array + k, dir, k, n_to_sort);
    }
}

/**
 * \brief Validate if array is sorted correctly. 
*/
void merge_sort(int *array, int size_sub, int index, int dir){
    if (index > 1){
        int k = size_sub / 2;

        merge_sort(array, index, k, 1);
        merge_sort(array, index + k, k, 0);
        merge_subarrays(array, index, size_sub, dir);
    }
    
}