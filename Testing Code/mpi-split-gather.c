// -----------------------------------------------------------------------------
// In this version each process only mallocs memory that it needs. It uses
// a gather at the end.
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
// Generates a 2D array of uninitialised doubles for a subsection of the plane
double** newSubPlane(int n, int rows) {
    double** plane  = ( double** )malloc(rows * sizeof(double*));
    plane[0] = ( double * )malloc(rows * n * sizeof(double));

    for(unsigned int i = 0; i<rows; i++)
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

// TODO propper doc string
// Populates the plane's wallswith the values provided, and sets the centre parts to zero. If debug is true then it prints outs the array
double** populateSubPlane(double** subPlane, int sizeOfPlane, int numRows, double top, double bottom, double farLeft, double farRight, int world_rank, int world_size)
{   
    // Generate 2d array of doubles
    for(int i=0; i<numRows; i++) {
        for(int j=0; j<sizeOfPlane; j++) {
            if(j == 0) {
                // Left
                subPlane[i][j] = farLeft;
            } else if(i == 0 && world_rank == 0) {
                // Top
                subPlane[i][j] = top;
            } else if(j == sizeOfPlane-1) {
                // Right
                subPlane[i][j] = farRight;
            } else if(i == numRows-1 && world_rank == world_size-1) {
                // Bottom
                subPlane[i][j] = bottom;
            } else {
                subPlane[i][j] = 0;
            }
        }
    }
    return subPlane;
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

void printSubPlane(double** plane, int sizeOfPlane, int numRows) {
    for(int x=0; x<numRows; x++) {
        for(int y=0; y<sizeOfPlane; y++) {
            printf("%f, ", plane[x][y]);
        }
        printf("\n");
    }
    printf("\n");
}

// TODO propper doc string
// Runs the relaxation technique on the 2d array of doubles that it is passed.
unsigned long relaxPlane(double** subPlane, int numRows, int sizeOfPlane, double tolerance, int world_rank, int world_size)
{

    unsigned long iterations = 0;
    int i, j, precisionFlag, endFlag;
    double pVal;
    MPI_Status status;

    int sizeOfInner = sizeOfPlane-2;
    int rowsPerThreadS = sizeOfInner/world_size+1;
    int rowsPerThreadE = sizeOfInner/world_size;
    int remainingRows = sizeOfInner - world_size * rowsPerThreadE;

    int sendBot, recBot;

    sendBot = numRows-2;
    recBot = numRows-1;

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

        for(i=1; i<recBot; i++) {
            for(j=((i+startingRow+1)%2)+1; j<sizeOfPlane-1; j+=2) {
                pVal = subPlane[i][j];
                subPlane[i][j] = (subPlane[i-1][j] + subPlane[i+1][j] + subPlane[i][j-1] + subPlane[i][j+1])/4;
                if(precisionFlag && tolerance < fabs(subPlane[i][j]-pVal)) {
                    precisionFlag = false;
                }
            }
        }
        
        // Update process that needs the new data
        if(world_rank==0) {
            // Only send data down
            MPI_Send(&subPlane[sendBot][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&subPlane[recBot][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
        } else if(world_rank==world_size-1) {
            // Only send data up
            MPI_Send(&subPlane[1][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD);
            MPI_Recv(&subPlane[0][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD, &status);
        } else {
            // Send data up and down
            MPI_Send(&subPlane[1][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD);
            MPI_Send(&subPlane[sendBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD);
            MPI_Recv(&subPlane[0][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&subPlane[recBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD, &status);
        }

        for(i=1; i<recBot; i++) {
            for(j=((i+startingRow)%2)+1; j<sizeOfPlane-1; j+=2) {
                pVal = subPlane[i][j];
                subPlane[i][j] = (subPlane[i-1][j] + subPlane[i+1][j] + subPlane[i][j-1] + subPlane[i][j+1])/4;
                if(precisionFlag && tolerance < fabs(subPlane[i][j]-pVal)) {
                    precisionFlag = false;
                }
            }
        }

        // Update process that needs the new data
        if(world_rank==0) {
            // Only send data down
            MPI_Send(&subPlane[sendBot][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&subPlane[recBot][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
        } else if(world_rank==world_size-1) {
            // Only send data up
            MPI_Send(&subPlane[1][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD);
            MPI_Recv(&subPlane[0][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD, &status);
        } else {
            // Send data up and down
            MPI_Send(&subPlane[1][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD);
            MPI_Send(&subPlane[sendBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD);
            MPI_Recv(&subPlane[0][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&subPlane[recBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD, &status);
        }

        MPI_Allreduce(&precisionFlag, &endFlag, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
        // if(iterations == 1)
        //     break;
    } while(!endFlag);

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
    double** subPlane;

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

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int sizeOfInner = sizeOfPlane-2;
    int rowsPerThreadS = sizeOfInner/world_size+1;
    int rowsPerThreadE = sizeOfInner/world_size;
    int remainingRows = sizeOfInner - world_size * rowsPerThreadE;

    int numRows, tempNumRows;

    if(world_rank < remainingRows) {
        numRows = rowsPerThreadS + 2;
    } else {
        numRows = rowsPerThreadE + 2;
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

    subPlane = newSubPlane(sizeOfPlane, numRows);

    subPlane = populateSubPlane(subPlane, sizeOfPlane, numRows, top, bottom, left, right, world_rank, world_size);

    if(!world_rank){
        plane = newPlane(sizeOfPlane);
        plane = populatePlane(plane, sizeOfPlane, left, right, top, bottom);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    iterations = relaxPlane(subPlane, numRows, sizeOfPlane, tolerance, world_rank, world_size);

    // Now gather all of the results
    MPI_Gatherv(&subPlane[1][0], (numRows-2)*sizeOfPlane, MPI_DOUBLE, &plane[1][0], recvcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if(debug && !world_rank) {
        printPlane(plane, sizeOfPlane);
    }

    MPI_Finalize();

    if(!world_rank) {
        printf("Threads: %d\n",world_size);
        printf("Size of Pane: %d\n", sizeOfPlane);
        printf("Iterations: %lu\n", iterations);
        printf("Time: %Lfs\n", toSeconds(start, end));
    }

    return 0;
}