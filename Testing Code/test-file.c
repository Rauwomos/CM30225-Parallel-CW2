#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
// #include <mpi.h>

#define BUFSIZE 10

int main(int argc, char **argv)
{
    int world_rank, world_size;

    double buf[BUFSIZE];
    char string_buf[318];
    char* output[BUFSIZE];

    for(int i=0; i<BUFSIZE; i++) {
        buf[i] = i;
        snprintf(string_buf, 318, "%f", buf[i]);
        output[i] = strdup(string_buf);
        printf("%s, ", output[i]);
    }

    

    // MPI_Init(&argc, &argv);
    // MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    // MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // MPI_File testfile;

    // MPI_File_open(MPI_COMM_WORLD, "testfile", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &testfile);

    // MPI_File_set_view(testfile, world_rank * BUFSIZE * sizeof(double), MPI_DOUBLE, MPI_DOUBLE, "native", MPI_INFO_NULL);

    // MPI_File_write(testfile, buf, BUFSIZE, MPI_DOUBLE, MPI_STATUS_IGNORE);

    // MPI_File_close(&testfile);

    // MPI_Finalize();

    // if(!world_rank)
        

    return 0;
}