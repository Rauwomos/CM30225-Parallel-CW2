#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <mpi.h>

float computeAvg(float* array, int array_size) {
    float sum = 0;
    for(int i=0; i<array_size; i++) {
        sum = sum + array[i];
    }
    return sum/array_size;
}

void printFloatArray(float* array, int array_size) {
    for(int i=0; i<array_size; i++) {
        printf("%f, ", array[i]);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    int world_rank, world_size;
    int elements_per_proc = 5;
    float* rand_nums;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_rank == 0) {
        rand_nums = malloc(sizeof(float) * elements_per_proc * world_size);
        for(int i=0; i<elements_per_proc*world_size; i++) {
            rand_nums[i] = i;
        }
        printFloatArray(rand_nums, elements_per_proc*world_size);
    }

// Create a buffer that will hold a subset of the random numbers
    float *sub_rand_nums = malloc(sizeof(float) * elements_per_proc);

// Scatter the random numbers to all processes
    MPI_Scatter(rand_nums, elements_per_proc, MPI_FLOAT, sub_rand_nums,
        elements_per_proc, MPI_FLOAT, 0, MPI_COMM_WORLD);

// Compute the average of your subset
    float sub_avg = computeAvg(sub_rand_nums, elements_per_proc);
// Gather all partial averages down to the root process
    float *sub_avgs = NULL;
    if (world_rank == 0) {
        sub_avgs = malloc(sizeof(float) * world_size);
    }
    MPI_Gather(&sub_avg, 1, MPI_FLOAT, sub_avgs, 1, MPI_FLOAT, 0,
        MPI_COMM_WORLD);

// Compute the total average of all numbers.
    if (world_rank == 0) {
        float avg = computeAvg(sub_avgs, world_size);
        printf("Avg: %f\n", avg);
    }

    MPI_Finalize();

    // if(!world_rank)
        

    return 0;
}