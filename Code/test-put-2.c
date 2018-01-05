#include "mpi.h"
#include "stdio.h"
/* This does a transpose with a get operation, fence, and derived
datatypes. Uses vector and hvector.  Run on 2 processes */
#define NROWS 100
#define NCOLS 100

int main(int argc, char *argv[])
{
    int rank, nprocs, flag;
    MPI_Win win;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Win_create(&flag, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    if (rank == 0) {

        MPI_Win_fence(0, win);
        for(int i=1; i<nprocs; i++) {
            flag = i;
            MPI_Put(&flag, 1, MPI_INT, i, 0, 1, MPI_INT, win);
        }
        MPI_Win_fence(0, win);
    } else { /* rank = 1 */
        MPI_Win_fence(0, win);
        MPI_Win_fence(0, win);
    }

    // for(int i=0; i<nprocs; i++) {
    //     if(rank == i)
            printf("process: %d Flag: %d\n", rank, flag);
    //     MPI_Barrier(MPI_COMM_WORLD);
    // }

    MPI_Win_free(&win);
    MPI_Finalize();
    return 0;
}