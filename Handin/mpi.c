#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <mpi.h>

int asprintf(char **strp, const char *fmt, ...);


/**
 * @brief Calculates the time in seconds between two timespec structs
 * @param start timespec struct with time less than end
 * @param end timespec struct with time greater than start
 * @return time difference between two struct
 */
long double toSeconds(struct timespec start, struct timespec end) {
    long long difSec = end.tv_sec - start.tv_sec;
    long long difNSec = end.tv_nsec - start.tv_nsec;
    long long totalNS = difSec*1000000000L + difNSec;
    return (long double) totalNS / 1000000000.0;
}

/**
 * @brief Mallocs memory for a n*rows 2D array. Memory for array is contiguous,
           for when I was using MPI_Gatherv
 * @param n the size of each row in the 2D array
 * @param rows the number of rows in the 2D array
 * @return a pointer to an array of pointers to each row in the 2D array
 */
double** newSubPlane(unsigned int n, unsigned int rows) {
    double** plane  = ( double** )malloc(rows * sizeof(double*));
    plane[0] = ( double * )malloc(rows * n * sizeof(double));

    for(unsigned int i = 0; i<rows; i++)
        plane[i] = (*plane + n * i);

    return plane;
}

/**
 * @brief Populates the plane's walls with the values provided, and sets the
 *         centre parts to zero
 * @param plane pointer to the 2D array
  * @param numRows number of rows in the array
 * @param sizeOfPlane length of each row in the array
 * @param top value to put in top edge of 2D array
 * @param bottom value to put in bottom edge of 2D array
 * @param farLeft value to put in left edge of 2D array
 * @param farRight value to put in right edge of 2D array
 * @param world_rank world_rank of this process
 * @param world_size number of MPI_processes
 */
void populateSubPlane(double** plane, int sizeOfPlane, int numRows, double top,
    double bottom, double farLeft, double farRight, int world_rank,
    int world_size)
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


/**
 * @brief Performs the relaxation algorithm on a 2D array
 * @param plane pointer to the 2D array
 * @param numRows number of rows in the array
 * @param sizeOfPlane length of each row in the array
 * @param tolerance the tolerance to perform the relaxation algorithm to
 * @param world_rank world_rank of this process
 * @param world_size number of MPI_processes
 * @return the number of iterations taken to perform the relaxation algorithm
 */
unsigned long relaxPlane(double** plane, int numRows, int sizeOfPlane,
    double tolerance, int world_rank, int world_size)
{

    unsigned long iterations = 0;
    int i, j, endFlag;
    double pVal;

    int sizeOfInner = sizeOfPlane-2;
    int sendBot = numRows-2; 
    int recBot = numRows-1;
    MPI_Request myRequest1, myRequest2;

    // Main Loop
    do {
        endFlag = true;
        iterations++;

        // Perform relaxation
        for(i=1; i<recBot; i++) {
            for(j=1; j<sizeOfPlane-1; j++) {
                pVal = plane[i][j];
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1]
                    + plane[i][j+1])/4;
                if(endFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    endFlag = false;
                }
            }
        }

        // MPI communication to Send/Recieve data depending on the world_rank
        if(world_rank==0) {
            // Only send data down to process with world_rank 1
            MPI_Isend(&plane[sendBot][1], sizeOfInner, MPI_DOUBLE, 1, 0,
                MPI_COMM_WORLD, &myRequest1);
            MPI_Recv(&plane[recBot][1], sizeOfInner, MPI_DOUBLE, 1, 0,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if(world_rank==world_size-1) {
            // Only send and recive/data to the process above i.e. world_rank-1
            MPI_Isend(&plane[1][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0,
                MPI_COMM_WORLD, &myRequest1);
            MPI_Recv(&plane[0][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            // Send new data up
            MPI_Isend(&plane[1][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0,
                MPI_COMM_WORLD, &myRequest1);
            // Send new data down 
            MPI_Isend(&plane[sendBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1,
                0, MPI_COMM_WORLD, &myRequest2);
            // Receive new data from above
            MPI_Recv(&plane[0][1], sizeOfInner, MPI_DOUBLE, world_rank-1, 0,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // Receive new data from below
            MPI_Recv(&plane[recBot][1], sizeOfInner, MPI_DOUBLE, world_rank+1,
                0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Compare and upate the endFlag for all of the MPI processes
        MPI_Allreduce(MPI_IN_PLACE, &endFlag, 1, MPI_INT, MPI_LAND,
            MPI_COMM_WORLD);
        
        
    } while(!endFlag);

    return iterations;
}

int main(int argc, char **argv)
{
    // Default values unless not set by command line flags
    int sizeOfPlane = 10;
    double tolerance = 0.00001;
    double left = 4;
    double right = 2;
    double top = 1;
    double bottom = 3;
    bool debug = false;

    int world_rank, world_size;

    // For timing algorithm
    struct timespec start, end;

    double** subPlane;

    unsigned long iterations;

    int opt;

    // Parse any command line flags
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

    int numRows;

    if(world_rank < remainingRows) {
        numRows = rowsPerThreadS + 2;
    } else {
        numRows = rowsPerThreadE + 2;
    }

    // Create new 2D array and populate 2D array with initial values
    subPlane = newSubPlane((unsigned int)sizeOfPlane, (unsigned int)numRows);
    populateSubPlane(subPlane, sizeOfPlane, numRows, top, bottom, left, right,
        world_rank, world_size);

    // Start timer 
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Run relaxation algorithm
    iterations = relaxPlane(subPlane, numRows, sizeOfPlane, tolerance,
        world_rank, world_size);

    // End Timer
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Output results to file
    if(debug) {
        FILE* file;
        char* file_name;
        /* Create the file name, so it is easy to see at a glance the world_size
            and problem size that the results came from */
        asprintf(&file_name, "%d-%d.result", world_size, sizeOfPlane);

        for(int i=0; i<world_size; i++) {
            // If it is this processes turn, write out to the file
            if (i == world_rank) {
                /* If this is world_rank 0 then create a new file,
                    otherwise append to the existing file */
                file = fopen(file_name, world_rank == 0 ? "w" : "a");
                
                /* Unless this is the first or last MPI process, do not write
                    out the first or last line of the array */
                int startingRow = 1;
                int endingRow = numRows - 1;
                
                /* If this is the first or last MPI process, then also write out
                    the first or last line of the array respecively */
                if(world_rank == 0) {
                    startingRow = 0;
                } else if(world_rank == world_size-1) {
                    endingRow = numRows;
                }
                // Write out data from the array
                for(int j=startingRow; j<endingRow; j++) {
                    for(int k=0; k<sizeOfPlane; k++)
                        fprintf(file, "%f, ", subPlane[j][k]);
                    fprintf(file, "\n");
                }
                fclose(file);
            }
            // Wait for MPI process that is writing to file
            MPI_Barrier(MPI_COMM_WORLD);
        }
        // Additional information about how the program ran
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

    // Print out some information about how the program ran to stdout 
    if(!world_rank) {
        printf("Threads: %d\n",world_size);
        printf("Size of Pane: %d\n", sizeOfPlane);
        printf("Iterations: %lu\n", iterations);
        printf("Time: %Lfs\n", toSeconds(start, end));
    }

    return 0;
}