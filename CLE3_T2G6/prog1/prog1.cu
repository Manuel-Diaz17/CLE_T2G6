
/**
 *  \file prog1.cu (implementation file)
 *
 *  \brief Problem name: Bitonic Sort Row Processing.
 *
 *
 *  \authors Manuel Diaz & Tiago Santos - June 2024
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include <cuda_runtime.h>

/**
 *   program configuration
 */

# define N 1024 * 1024

# define K 1024


/* returns the number of seconds elapsed between the two specified times */
static double get_delta_time(void);

/* returns 1 if the specified array is sorted, and 0 otherwise */
int validateSort(int *arr, int n);

/* Function to merge two haves of array */
__device__ void merge(int arr[], int l, int m, int r);

/* Iterative mergesort */
__device__ void merge_sort(int arr[], int n);

/* kernel function */
__global__ void process_sequence_sorting(int *data, int iter);


/**
 *  \brief Function merge.
 *
 *  This function merges two sorted subarrays into a single sorted subarray.
 *
 *  \param array: pointer to the array containing the subarrays
 *  \param left: starting index of the first subarray
 *  \param mid: ending index of the first subarray and starting index of the second subarray
 *  \param right: ending index of the second subarray
 *
 *  The function creates temporary arrays to store the subarrays and then merges them into the original 
 *  array in a sorted order.
 */
__device__ void merge(int array[], int left, int mid, int right)
{
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 =  right - mid;

	int *L = (int*)malloc(n1 * sizeof(int));
	int *R = (int*)malloc(n2 * sizeof(int));
 
    // Copy data to temporary arrays
    for (i = 0; i < n1; i++)
        L[i] = array[left + i];
    for (j = 0; j < n2; j++)
        R[j] = array[mid + 1+ j];
 
    // Merge temporary arrays into arr
    i = 0;
    j = 0;
    k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            array[k] = L[i];
            i++;
        } else {
            array[k] = R[j];
            j++;
        }
        k++;
    }
 
    // Copy remaining elements of L[]
    while (i < n1) {
        array[k] = L[i];
        i++;
        k++;
    }
 
    // Copy remaining elements of R[]
    while (j < n2) {
        array[k] = R[j];
        j++;
        k++;
    }

	free(L);
	free(R);
}

/**
 *  \brief Function merge_sort.
 *
 *  This function sorts an array using the merge sort algorithm.
 *
 *  \param array: pointer to the array to be sorted
 *  \param size: size of the array
 *
 *  The function divides the array into smaller subarrays and recursively sorts them using merge sort. 
 *  It then merges the sorted subarrays to obtain the final sorted array.
 */
__device__ void merge_sort(int array[], int size) {
   int currentSize, leftStart;
	
	for (currentSize = 1; currentSize <= size - 1; currentSize = 2 * currentSize) {
		for (leftStart = 0; leftStart < size - 1; leftStart += 2 * currentSize) {
           int middle = min(leftStart + currentSize - 1, size - 1);
           int rightEnd = min(leftStart + 2 * currentSize - 1, size - 1);
           merge(array, leftStart, middle, rightEnd);
       	}
   	}
}

/**
 *  \brief Function process_sequence_sorting.
 *
 *  This CUDA kernel function performs parallel processing on the input array using merge sort algorithm.
 *
 *  \param data: pointer to the input array
 *  \param iter: iteration number indicating the level of merge sort
 *
 *  The function divides the input array into subsequences and sorts them using merge sort.
 *  Each thread is responsible for sorting a specific subsequence.
 *  In each iteration, the function performs either an independent merge sort on a subsequence (when iter is 0) 
 *  or merges two previously sorted subsequences.
 */
__global__ void process_sequence_sorting(int *data, int iter) {
	//int N = DIM;
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int idx = blockDim.x * gridDim.x * y + x;

    int limit = K >> iter;
	if(idx >= limit)
        return;
    
	int start = N/K * (1 << iter) * idx;
	int end = start + (1 << iter) * N/K;
	int mid = (start + end) / 2;
	int subseq_len = (1 << iter) * N/K;
	int *subseq_start = data + start;

	(iter == 0) ? merge_sort(subseq_start, subseq_len) : merge(data, start, mid-1, end-1);
    __syncthreads();
}

/**
 *  \brief Validate Sort.
 *
 *  This function checks if an array is sorted in ascending order.
 *
 *  \param arr: pointer to the array to be validated
 *  \param n: size of the array
 *
 */
int validateSort(int *arr, int n) {
    int i;

    for (i = 0; i < n - 1; i++)
    {
        if (arr[i] > arr[i + 1])
        {
            printf("Error in position %d between element %d and %d\n", i, arr[i], arr[i + 1]);
            return 0;
        }
    }
	if (i == (n - 1))
		printf("Everything is OK!\n");

    return 1;
}


/**
 *  \brief Main function.
 *
 *  \param argc: number of command-line arguments
 *  \param argv: array of command-line argument strings
 *
 *  The function reads an input file containing integers, performs parallel merge sort using CUDA, 
 *  and validates the sorted array.
 */
int main (int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	/* Open the file for reading */

	FILE *file = fopen(argv[1], "rb");
	if (file == NULL) {
		printf("Failed to open file: %s\n", argv[1]);
		return 1;
	}

	fseek(file, 0, SEEK_END);
	int size = ftell(file) / sizeof(int);
	fseek(file, 0, SEEK_SET);

	int *host_matrix = (int*) malloc(size * sizeof(int));
	if (host_matrix == NULL) {
		printf("Error: cannot allocate memory\n");
		return 1;
	}

	int count = fread(host_matrix, sizeof(int), size, file);

	if (count != size) {
		printf("Error: could not read all integers from file\n");
		return 1;
	}

	fclose(file);


	/* set up the device */

	int dev = 0;

	cudaDeviceProp deviceProp;
	CHECK (cudaGetDeviceProperties (&deviceProp, dev));
	printf("Using Device %d: %s\n", dev, deviceProp.name);
	CHECK (cudaSetDevice (dev));

	/* copy the host data to the device memory */
	int *device_matrix;
	CHECK(cudaMalloc((void**)&device_matrix, N * sizeof(int)));
	CHECK(cudaMemcpy(device_matrix, host_matrix, K * sizeof(int[K]), cudaMemcpyHostToDevice));


	/* launch the kernel */

	int gridDimX, gridDimY, gridDimZ, blockDimX, blockDimY, blockDimZ;

	// Number of threads in each dimension of a block
	blockDimX = 1 << 0;                                             // optimize!
	blockDimY = 1 << 0;                                             // optimize!
	blockDimZ = 1 << 0;                                             // do not change!

	// Number of blocks in each dimension of the grid
	gridDimX = K;													// optimize!
	gridDimY = 1 << 0;												// optimize!
	gridDimZ = 1 << 0;                                              // do not change!

	dim3 grid (gridDimX, gridDimY, gridDimZ);
	dim3 block (blockDimX, blockDimY, blockDimZ);

	if ((gridDimX * gridDimY * gridDimZ * blockDimX * blockDimY * blockDimZ) != K) {
		printf ("Wrong configuration!\n");
		printf("blockDimX = %d, blockDimY = %d, blockDimZ = %d\n", blockDimX, blockDimY, blockDimZ);
		printf("gridDimX = %d, gridDimY = %d, gridDimZ = %d\n", gridDimX, gridDimY, gridDimZ);
		return 1;
	}

	// Perform merge sort
	(void) get_delta_time ();

    int iter = 0; 
    int size2 = (N / K) * (1 << iter);

	if (K == 1) {
		printf("Iteration = %d\n", iter);

		process_sequence_sorting<<<grid, block>>>(device_matrix, iter);
		gridDimX = K / (1 << (iter + 1));
	}
	else {
		for (int iter = 0; size2 < N; iter++) {
        	printf("Iteration = %d\n", iter);

			process_sequence_sorting<<<grid, block>>>(device_matrix, iter);
			gridDimX = K / (1 << (iter + 1));  // Divides by 2 each iteration

			dim3 grid (gridDimX, gridDimY, gridDimZ);
        	size2 = (N / K) * (1 << iter);

			CHECK (cudaDeviceSynchronize ());                            // wait for kernel to finish
			CHECK (cudaGetLastError ());                                 // check for kernel errors
		}
	}
	
	CHECK (cudaDeviceSynchronize ());                            // wait for kernel to finish
	CHECK (cudaGetLastError ());                                 // check for kernel errors

	printf("The CUDA kernel <<<(%d,%d,%d), (%d,%d,%d)>>> took %.3f seconds to run\n",
			gridDimX, gridDimY, gridDimZ, blockDimX, blockDimY, blockDimZ, get_delta_time ());

	/* copy kernel result back to host side */
	CHECK (cudaMemcpy (host_matrix, device_matrix, K * sizeof(int[K]), cudaMemcpyDeviceToHost));

	/* free device global memory */
	CHECK (cudaFree (device_matrix));

	/* reset the device */
	CHECK (cudaDeviceReset ());

	// validate if the array is sorted correctly
	validateSort(host_matrix, K);
	free(host_matrix);
	return 0;
}

/**
 *  \brief Get delta time.
 *
 *  This function measures the elapsed time between successive calls.
 *
 *  \return The time elapsed between successive calls in seconds.
 *
 *  The function uses the CLOCK_MONOTONIC clock to measure time.
 */
static double get_delta_time(void)
{
	static struct timespec t0,t1;

	t0 = t1;
	if(clock_gettime(CLOCK_MONOTONIC,&t1) != 0)
	{
		perror("clock_gettime");
		exit(1);
	}
	return (double)(t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double)(t1.tv_nsec - t0.tv_nsec);
}