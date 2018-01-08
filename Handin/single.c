#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

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
 * @brief Mallocs memory for a n*n 2D array
 * @param n the size of each side of the array
 * @return a pointer to an array of pointers to each row in the 2D array
 */
double** newPlane(unsigned int n) {
    double** plane = ( double** )malloc(n * sizeof(double*));
    for (unsigned int i = 0; i < n; ++i)
        plane[i] = ( double* )malloc(n * sizeof(double));
    return plane;
}

/**
 * @brief Populates the plane's walls with the values provided, and sets the
 *         centre parts to zero
 * @param plane pointer to the 2D array
 * @param sizeOfPlane number of rows and length of each row
 * @param top value to put in top edge of 2D array
 * @param bottom value to put in bottom edge of 2D array
 * @param farLeft value to put in left edge of 2D array
 * @param farRight value to put in right edge of 2D array
 */
void populatePlane(double** plane, unsigned int sizeOfPlane, double top,
    double bottom, double farLeft, double farRight)
{   
    // Fill 2D array with correct values
    for(unsigned int j=0; j<sizeOfPlane; j++) {
        for(unsigned int i=0; i<sizeOfPlane; i++) {
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
                // Centre 
                plane[i][j] = 0;
            }
        }
    }
}

/**
 * @brief Prints out a 2D array to stdout
 * @param plane pointer to the 2D array
 * @param sizeOfPlane number of rows and length of each row in the array
 */
void printPlane(double** plane, unsigned int sizeOfPlane) {
    for(unsigned int x=0; x<sizeOfPlane; x++) {
        for(unsigned int y=0; y<sizeOfPlane; y++) {
            printf("%f, ", plane[x][y]);
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * @brief Performs the relaxation algorithm on a 2D array
 * @param plane pointer to the 2D array
 * @param sizeOfPlane number of rows and length of each row in the array
 * @param tolerance the tolerance to perform the relaxation algorithm to
 * @return the number of iterations taken to perform the relaxation algorithm
 */
unsigned long relaxPlane(double** plane, unsigned int sizeOfPlane,
    double tolerance)
{
    unsigned long iterations = 0;
    unsigned int i,j;
    double pVal;
    bool endFlag;

    // Main loop
    while (1) {
        // Reset the flag
        endFlag = true;
        // Increment iteration counter
        iterations++;

        for(i=1; i<sizeOfPlane-1; i++) {
            for(j=1; j<sizeOfPlane-1; j++) {
                // Temporarily store previous value
                pVal = plane[i][j];
                // Calulate new value
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1]
                    + plane[i][j+1])/4;
                /* If the endflag is still true and the absolute differnece
                 * between the old and new value is greater than the
                 * tolerance, set the flag to false */
                if(endFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    endFlag = false;
                }
            }
        }

        // It the flag was not set to false this iteration, then it is finished
        if(endFlag)
            break;
    }

    return iterations;
}

int main(int argc, char **argv)
{
    // Default values unless not set by command line flags
    unsigned int sizeOfPlane = 10;
    double tolerance = 0.00001;
    double left = 4;
    double right = 2;
    double top = 1;
    double bottom = 3;
    bool debug = false;

    // For timing algorithm
    struct timespec start, end;

    double** plane;

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
                sizeOfPlane = (unsigned int) atoi(optarg);
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
            default:
                fprintf (stderr, "Unknown option `\\x%x'.\n", optopt);
                return 1;
    }

    // Size of the plane must be at least 3x3 or ends with exit code 1
    if(sizeOfPlane < 3) {
        fprintf (stderr, "The size of the plane must be greater than 2\n");
        return 1;
    }
    // Tolerance must be greater than 0, or ends with exit code 1
    if(tolerance < 0) {
        fprintf (stderr, "The tolerance must be greater than 0\n");
        return 1;
    }

    // Create 2D array and populate values
    plane = newPlane(sizeOfPlane);
    populatePlane(plane, sizeOfPlane, left, right, top, bottom);

    // Start timer
    clock_gettime(CLOCK_MONOTONIC, &start);
    // Perform relaxation algorithm
    iterations = relaxPlane(plane, sizeOfPlane, tolerance);
    // End timer
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Print out plane if debug is true
    if(debug)
        printPlane(plane, sizeOfPlane);

    // Print out information about how the program ran
    printf("Threads: 1\n");
    printf("Size of Pane: %d\n", sizeOfPlane);
    printf("Iterations: %lu\n", iterations);
    printf("Time: %Lfs\n", toSeconds(start, end));
    return 0;
}