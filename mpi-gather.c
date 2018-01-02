// -----------------------------------------------------------------------------
// This version does not use scatter or gather. It is also not very memory 
// efficient as every program mallocs memory for a full plane. While this is not
// good practice, it is not an issue for this program as the ammount of memory
// being used to store a plane is relatively small.
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <mpi.h>


long double toSeconds(struct timespec start, struct timespec end) {
    long long difSec = end.tv_sec - start.tv_sec;
    long long difNSec = end.tv_nsec - start.tv_nsec;
    long long totalNS = difSec*1000000000L + difNSec;
    return (long double) totalNS / 1000000000.0;
}

// TODO propper doc string
// Generates a 2D array of uninitialised doubles
double** newPlane(unsigned int n) {
    double** plane  = ( double** )malloc(n * sizeof(double*));
    plane[0] = ( double * )malloc(n * n * sizeof(double));
 
    for(unsigned int i = 0; i < n; i++)
        plane[i] = (*plane + n * i);

    return plane;
}

// TODO propper doc string
// Populates the plane's wallswith the values provided, and sets the centre parts to zero. If debug is true then it prints outs the array
double** populatePlane(double** plane, int sizeOfPlane, double left, double right, double top, double bottom)
{   
    // Generate 2d array of doubles
    for(int i=0; i<sizeOfPlane; i++) {
        for(int j=0; j<sizeOfPlane; j++) {
            if(i == 0) {
                // Top
                plane[i][j] = top;
            } else if(j == 0) {
                // Left
                plane[i][j] = left;
            } else if(i == sizeOfPlane-1) {
                // Bottom
                plane[i][j] = bottom;
            } else if(j == sizeOfPlane-1) {
                // Right
                plane[i][j] = right;
            } else {
                plane[i][j] = 0;
            }
        }
    }
    return plane;
}

void printPlane(double** plane, int sizeOfPlane) {
    for(int x=0; x<sizeOfPlane; x++) {
        for(int y=0; y<sizeOfPlane; y++) {
            printf("%f, ", plane[x][y]);
        }
        printf("\n");
    }
    printf("\n");
}

// TODO propper doc string
// Runs the relaxation technique on the 2d array of doubles that it is passed.
unsigned long relaxPlane(double** plane, int sizeOfPlane, double tolerance, int world_rank, int world_size)
{
    unsigned long iterations = 0;
    int i, j, precisionFlag, endFlag;
    double pVal;
    MPI_Status status;

    int sizeOfInner = sizeOfPlane-2;
    int rowsPerThreadS = sizeOfInner/world_size+1;
    int rowsPerThreadE = sizeOfInner/world_size;
    int remainingRows = sizeOfInner - world_size * rowsPerThreadE;

    int startingRow, endingRow;

    if(world_rank < remainingRows) {
        startingRow = world_rank * rowsPerThreadS + 1;
        endingRow = startingRow + rowsPerThreadS;
    } else {
        startingRow = world_rank * rowsPerThreadE + remainingRows + 1;
        endingRow = startingRow + rowsPerThreadE;
    }

    // printf("Process: %d SE: %d, %d\n", world_rank, startingRow, endingRow);

    do {
        precisionFlag = true;
        iterations++;

        for(i=startingRow; i<endingRow; i++) {
            for(j=(i%2)+1; j<sizeOfPlane-1; j+=2) {
                pVal = plane[i][j];
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1] + plane[i][j+1])/4;
                if(precisionFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    precisionFlag = false;
                }
            }
        }
        
        // Update process that needs the new data
        if(world_rank==0) {
            // Only send data down
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
        } else if(world_rank==world_size-1) {
            // Only send data up
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD, &status);
        } else {
            // Send data up and down
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD);
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD, &status);
        }

        for(i=startingRow; i<endingRow; i++) {
            for(j=((i+1)%2)+1; j<sizeOfPlane-1; j+=2) {
                pVal = plane[i][j];
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1] + plane[i][j+1])/4;
                if(precisionFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    precisionFlag = false;
                }
            }
        }

        // Update process that needs the new data
        if(world_rank==0) {
            // Only send data down
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
        } else if(world_rank==world_size-1) {
            // Only send data up
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD, &status);
        } else {
            // Send data up and down
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD);
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD, &status);
        }

        MPI_Allreduce(&precisionFlag, &endFlag, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

    } while(!endFlag);

    // Simple way to bring together all the data. With a bit of math I could make this a lot faster.
    // if(world_rank == 0) {
    //     for(i=endingRow; i<sizeOfPlane-1; i++) {
    //         MPI_Recv(&plane[i][1], sizeOfInner, MPI_DOUBLE, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &status);
    //     }
    // } else {
    //     for(i=startingRow; i<endingRow; i++) {
    //         MPI_Send(&plane[i][1], sizeOfInner, MPI_DOUBLE, 0, i, MPI_COMM_WORLD);        
    //     }
    // }

    return iterations;
}

int main(int argc, char **argv)
{
    int sizeOfPlane = 10;
    double tolerance = 0.00001;
    double left = 4;
    double right = 2;
    double top = 1;
    double bottom = 3;

    int world_rank, world_size;

    // For timing algorithm
    struct timespec start, end;

    double** plane;

    unsigned long iterations;

    int opt;
    bool debug = false;

    while ((opt = getopt (argc, argv, "u:d:l:r:s:p:h:x")) != -1)
        switch (opt) {
            case 'u':
                top = atof(optarg);
                break;
            case 'd':
                bottom = atof(optarg);
                break;
            case 'l':
                left = atof(optarg);
                break;
            case 'r':
                right = atof(optarg);
                break;
            case 's':
                sizeOfPlane = (int) atoi(optarg);
                break;
            case 'p':
                tolerance = atof(optarg);
                break;
            case 'h':
                // TODO print help stuff
                printf("TODO help info\n");
                break;
            case 'x':
                debug = true;
                break;
            case '?':
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
    }

    // Size of the plane must be at least 3x3
    if(sizeOfPlane < 3) {
        fprintf (stderr, "The size of the plane must be greater than 2\n");
        return 1;
    }
    // Tolerance must be greater than 0
    if(tolerance < 0) {
        fprintf (stderr, "The tolerance must be greater than 0\n");
        return 1;
    }


    plane = newPlane(sizeOfPlane);
    plane = populatePlane(plane, sizeOfPlane, left, right, top, bottom);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int sizeOfInner = sizeOfPlane-2;
    int rowsPerThreadS = sizeOfInner/world_size+1;
    int rowsPerThreadE = sizeOfInner/world_size;
    int remainingRows = sizeOfInner - world_size * rowsPerThreadE;

    int startingRow, numRows, tempNumRows;

    if(world_rank < remainingRows) {
        startingRow = world_rank * rowsPerThreadS + 1;
    } else {
        startingRow = world_rank * rowsPerThreadE + remainingRows + 1;
    }

    if(world_rank < remainingRows) {
        numRows = rowsPerThreadS;
    } else {
        numRows = rowsPerThreadE;
    }

    int* recvcounts = malloc(world_size * sizeof(int)); 
    int* displs = malloc(world_size * sizeof(int));

    // Calculate recvcounts and displs 
    for(int i=0; i<world_size; i++) {

        if(i < remainingRows) {
            tempNumRows = rowsPerThreadS;
        } else {
            tempNumRows = rowsPerThreadE;
        }

        recvcounts[i] = tempNumRows * sizeOfPlane;
        if(i < remainingRows) {
            displs[i] = (i * rowsPerThreadS) * sizeOfPlane;
        } else {
            displs[i] = (i * rowsPerThreadE + remainingRows) * sizeOfPlane;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    iterations = relaxPlane(plane, sizeOfPlane, tolerance, world_rank, world_size);

    MPI_Gatherv(&plane[startingRow][0], numRows*sizeOfPlane, MPI_DOUBLE, &plane[1][0], recvcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);    

    clock_gettime(CLOCK_MONOTONIC, &end);

    if(debug && !world_rank)
        printPlane(plane, sizeOfPlane);

    MPI_Finalize();

    if(!world_rank) {
        printf("Threads: %d\n",world_size);
        printf("Size of Pane: %d\n", sizeOfPlane);
        printf("Iterations: %lu\n", iterations);
        printf("Time: %Lfs\n", toSeconds(start, end));
    }

    return 0;
}