#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <mpi.h>

int asprintf(char **strp, const char *fmt, ...);


/**
 * @brief Calculates the time difference in seconds between two timespec structs
 * @return the time difference in seconds between the two structs
 */
long double toSeconds(struct timespec start, struct timespec end) {
    long long difSec = end.tv_sec - start.tv_sec;
    long long difNSec = end.tv_nsec - start.tv_nsec;
    long long totalNS = difSec*1000000000L + difNSec;
    return (long double) totalNS / 1000000000.0;
}

// TODO correct this docstring
/**
 * @brief Mallocs memory for a n*rows 2D array
 * @param n the size of each row in the 2D array
 * @param rows the number of rows in the 2D array
 * @return a pointer to the 2D array
 */
double** newSubPlane(unsigned int n, unsigned int rows) {
    double** plane  = ( double** )malloc(rows * sizeof(double*));
    plane[0] = ( double * )malloc(rows * n * sizeof(double));

    for(unsigned int i = 0; i<rows; i++)
        plane[i] = (*plane + n * i);

    return plane;
}

/**
 * @brief Populates the plane's walls with the values provided, and zero's out the rest.
 */
void populateSubPlane(double** plane, int sizeOfPlane, int numRows, double top, double bottom, double farLeft, double farRight, int world_rank, int world_size)
{   
    for(int i=0; i<numRows; i++) {
        for(int j=0; j<sizeOfPlane; j++) {
            if(j == 0) {
                // Left
                plane[i][j] = farLeft;
            } else if(i == 0 && world_rank == 0) {
                // Top
                plane[i][j] = top;
            } else if(j == sizeOfPlane-1) {
                // Right
                plane[i][j] = farRight;
            } else if(i == numRows-1 && world_rank == world_size-1) {
                // Bottom
                plane[i][j] = bottom;
            } else {
                plane[i][j] = 0;
            }
        }
    }
}

// TODO propper doc string
// Runs the relaxation technique on the 2d array of doubles that it is passed.
unsigned long relaxPlane(double** plane, int numRows, int sizeOfPlane, double tolerance, int world_rank, int world_size)
{

    unsigned long iterations = 0;
    int i, j, endFlag;
    double pVal;

    int sizeOfInner = sizeOfPlane-2;
    int sendBot = numRows-2; 
    int recBot = numRows-1;
    MPI_Request myRequest1, myRequest2, myRequest3, myRequest4;

    do {
        endFlag = true;
        iterations++;

        for(i=1; i<recBot; i++) {
            for(j=1; j<sizeOfPlane-1; j++) {
                pVal = plane[i][j];
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1] + plane[i][j+1])/4;
                if(endFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    endFlag = false;
                }
            }
        }

        // Update process that needs the new data
        if(world_rank==0) {
            // printf("Process: %d about to send\n" ,world_rank);
            // Only send data down
            MPI_Isend(&plane[sendBot][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &myRequest1);
            // printf("Process: %d finished sending\n" ,world_rank);
            MPI_Recv(&plane[recBot][1], sizeOfInner, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // printf("Process: %d finished communicating\n" ,world_rank);
        } else if(world_rank==world_size-1) {
            // printf("Process: %d about to send\n" ,world_rank);
            // Only send data up
            MPI_Isend(&plane[1][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD, &myRequest1);
            // printf("Process: %d finished sending\n" ,world_rank);
            MPI_Recv(&plane[0][1], sizeOfInner, MPI_DOUBLE, world_size-2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // printf("Process: %d finished communicating\n" ,world_rank);
        } else {
            // printf("Process: %d about to send\n" ,world_rank);
            // Send data up and down
            MPI_Isend(&plane[1][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD, &myRequest1);
            // printf("Process: %d finished send 1\n" ,world_rank);
            MPI_Isend(&plane[sendBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD, &myRequest2);
            // printf("Process: %d finished sending\n" ,world_rank);
            MPI_Recv(&plane[0][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&plane[recBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // printf("Process: %d finished communicating\n" ,world_rank);
        }

        MPI_Allreduce(MPI_IN_PLACE, &endFlag, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

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

    int* recvcounts = malloc((unsigned int)world_size * sizeof(int)); 
    int* displs = malloc((unsigned int)world_size * sizeof(int));

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

    subPlane = newSubPlane((unsigned int)sizeOfPlane, (unsigned int)numRows);

    subPlane = populateSubPlane(subPlane, sizeOfPlane, numRows, top, bottom, left, right, world_rank, world_size);

    clock_gettime(CLOCK_MONOTONIC, &start);
    iterations = relaxPlane(subPlane, numRows, sizeOfPlane, tolerance, world_rank, world_size);

    // Now gather all of the results
    // MPI_Gatherv(&subPlane[1][0], (numRows-2)*sizeOfPlane, MPI_DOUBLE, &plane[1][0], recvcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if(debug) {
        FILE* file;
        char* file_name;

        asprintf(&file_name, "%d-%d.result", world_size, sizeOfPlane);

        for(int i=0; i<world_size; i++) {
            if (i == world_rank) {
                file = fopen(file_name, world_rank == 0 ? "w" : "a");
                int startingRow = 1;
                int endingRow = numRows - 1;
                
                if(world_rank == 0) {
                    startingRow = 0;
                } else if(world_rank == world_size-1) {
                    endingRow = numRows;
                }
                
                for(int j=startingRow; j<endingRow; j++) {
                    for(int k=0; k<sizeOfPlane; k++) {
                        fprintf(file, "%f, ", subPlane[j][k]);
                    }
                    fprintf(file, "\n");
                }
                fclose(file);
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
        if(!world_rank) {
            file = fopen(file_name, "a");
            fprintf(file, "\nThreads: %d\n",world_size);
            fprintf(file, "Size of Pane: %d\n", sizeOfPlane);
            fprintf(file, "Iterations: %lu\n", iterations);
            fprintf(file, "Time: %Lfs\n", toSeconds(start, end));
            fclose(file);
        }
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