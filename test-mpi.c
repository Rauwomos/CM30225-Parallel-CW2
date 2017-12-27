#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int world_rank, world_size;

    int test = 1;
    int test2 = 1;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if(world_rank == 1)
        test = false;

    printf("Before Reduce; Threads: %d ID: %d Bool: %d\n", world_size, world_rank, test);

    MPI_Allreduce(&test, &test2, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
    // MPI_Reduce(&test, &test2, 1, MPI_INT, MPI_LAND, 0, MPI_COMM_WORLD);

    printf("After Reduce; Threads: %d ID: %d Bool: %d Bool2 %d\n", world_size, world_rank, test, test2);

    MPI_Finalize();

    if(!world_rank)
        printf("Bool2: %d\n", test2);

    return 0;
}