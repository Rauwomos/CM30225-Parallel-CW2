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
double** newPlane(int n) {
    double** plane = ( double** )malloc(((unsigned int) n) * sizeof(double*));
    for (int i = 0; i < n; ++i)
        plane[i] = ( double* )malloc(((unsigned int) n) * sizeof(double));
    return plane;
}

// TODO propper doc string
// Populates the plane's wallswith the values provided, and sets the centre parts to zero. If debug is true then it prints outs the array
double** populatePlane(double** plane, int sizeOfPlane, double top, double bottom, double farLeft, double farRight)
{   
    // Generate 2d array of doubles
    for(int j=0; j<sizeOfPlane; j++) {
        for(int i=0; i<sizeOfPlane; i++) {
            if(i == 0) {
                // Left
                plane[i][j] = farLeft;
            } else if(j == 0) {
                // Top
                plane[i][j] = top;
            } else if(i == sizeOfPlane-1) {
                // Right
                plane[i][j] = farRight;
            } else if(j == sizeOfPlane-1) {
                // Bottom
                plane[i][j] = bottom;
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
unsigned long relaxPlane(double** plane, int sizeOfPlane, double tolerance, int id, int numProcess)
{
    unsigned long iterations = 0;
    int i,j;
    double pVal;
    bool endFlag;
    bool mpiVal;
    MPI_Status status;

    int sizeOfInner = sizeOfPlane-2;
    int rowsPerThreadS = sizeOfInner/numProcess+1;
    int rowsPerThreadE = sizeOfInner/numProcess;
    int remainingRows = sizeOfInner - numProcess * rowsPerThreadE;

    int startingRow, endingRow;

    if(id < remainingRows) {
        startingRow = id * rowsPerThreadS + 1;
        endingRow = startingRow + rowsPerThreadS;
    } else {
        startingRow = id * rowsPerThreadE + remainingRows + 1;
        endingRow = startingRow + rowsPerThreadE;
    }

    // printf("Process: %d SE: %d, %d\n", id, startingRow, endingRow);

    do {
        endFlag = true;
        iterations++;

        for(i=startingRow; i<endingRow; i++) {
            for(j=(i%2)+1; j<sizeOfPlane-1; j+=2) {
                pVal = plane[i][j];
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1] + plane[i][j+1])/4;
                // plane[i][j] = id+1;
                if(endFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    endFlag = false;
                }
            }
        }
        
        // Update process that needs the new data
        if(id==0) {
            // Only send data down
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
        } else if(id==numProcess-1) {
            // Only send data up
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, numProcess-2, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, numProcess-2, 0, MPI_COMM_WORLD, &status);
        } else {
            // Send data up and down
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, id-1, 0, MPI_COMM_WORLD);
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, id+1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, id-1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, id+1, 0, MPI_COMM_WORLD, &status);
        }
        
        // if(id == 1) {
        //     printPlane(plane, sizeOfPlane);
        //     printf("\n");
        // }

        for(i=startingRow; i<endingRow; i++) {
            for(j=((i+1)%2)+1; j<sizeOfPlane-1; j+=2) {
                pVal = plane[i][j];
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1] + plane[i][j+1])/4;
                // plane[i][j] = id+1;
                if(endFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    endFlag = false;
                }
            }
        }

        // Update process that needs the new data
        if(id==0) {
            // Only send data down
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
            for(i=1; i<numProcess; i++) {
                MPI_Recv(&mpiVal, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
                if(!mpiVal) {
                    endFlag = false;
                }
            }
        } else if(id==numProcess-1) {
            // Only send data up
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, numProcess-2, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, numProcess-2, 0, MPI_COMM_WORLD, &status);
            MPI_Send(&endFlag, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        } else {
            // Send data up and down
            MPI_Send(&plane[startingRow][1], sizeOfInner, MPI_DOUBLE, id-1, 0, MPI_COMM_WORLD);
            MPI_Send(&plane[endingRow-1][1], sizeOfInner, MPI_DOUBLE, id+1, 0, MPI_COMM_WORLD);
            MPI_Recv(&plane[startingRow-1][1], sizeOfInner, MPI_DOUBLE, id-1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&plane[endingRow][1], sizeOfInner, MPI_DOUBLE, id+1, 0, MPI_COMM_WORLD, &status);
            MPI_Send(&endFlag, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        }

        // if(id == 1) {
        //     printPlane(plane, sizeOfPlane);
        //     printf("\n");
        // }

        MPI_Bcast(&endFlag, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // if(iterations == 2)
        //     break;
    } while(!endFlag);

    // Simple way to bring together all the data. With a bit of math I could make this a lot faster.
    if(id == 0) {
        for(i=endingRow; i<sizeOfPlane-1; i++) {
            MPI_Recv(&plane[i][1], sizeOfInner, MPI_DOUBLE, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &status);
        }
    } else {
        for(i=startingRow; i<endingRow; i++) {
            MPI_Send(&plane[i][1], sizeOfInner, MPI_DOUBLE, 0, i, MPI_COMM_WORLD);        
        }
    }

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

    int id, numProcess;

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
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcess);

    clock_gettime(CLOCK_MONOTONIC, &start);
    iterations = relaxPlane(plane, sizeOfPlane, tolerance, id, numProcess);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if(debug && !id)
        printPlane(plane, sizeOfPlane);

    MPI_Finalize();

    if(!id) {
        printf("Threads: %d\n",numProcess);
        printf("Size of Pane: %d\n", sizeOfPlane);
        printf("Iterations: %lu\n", iterations);
        printf("Time: %Lfs\n", toSeconds(start, end));
    }

    return 0;
}