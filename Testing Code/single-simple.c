#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>


// Given two tm structs, it returns the time difference in seconds
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
 * @brief Populates the plane's walls with the values provided, and sets the centre parts to zero
 * @param plane pointer to the 2D array
 * @param sizeOfPlane number of rows and length of each row
 * @param top value to put in top edge of 2D array
 * @param bottom value to put in bottom edge of 2D array
 * @param farLeft value to put in left edge of 2D array
 * @param farRight value to put in right edge of 2D array
 * @return a pointer to an array of pointers to each row in the 2D array
 */
void populatePlane(double** plane, unsigned int sizeOfPlane, double top, double bottom, double farLeft, double farRight)
{   
    // Generate 2d array of doubles
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
                plane[i][j] = 0;
            }
        }
    }
    return plane;
}

void printPlane(double** plane, unsigned int sizeOfPlane) {
    for(unsigned int x=0; x<sizeOfPlane; x++) {
        for(unsigned int y=0; y<sizeOfPlane; y++) {
            printf("%f, ", plane[x][y]);
        }
        printf("\n");
    }
    printf("\n");
}

// TODO propper doc string
// Runs the relaxation technique on the 2d array of doubles that it is passed.
unsigned long relaxPlane(double** plane, unsigned int sizeOfPlane, double tolerance)
{
    unsigned long iterations = 0;
    unsigned int i,j;
    double pVal;
    bool endFlag;
    while (1) {
        endFlag = true;
        iterations++;
        // i then j, accessing continues blocks of memory. Increases speed by 1/3
        for(i=1; i<sizeOfPlane-1; i++) {
            for(j=1; j<sizeOfPlane-1; j++) {
                pVal = plane[i][j];
                plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1] + plane[i][j+1])/4;
                if(endFlag && tolerance < fabs(plane[i][j]-pVal)) {
                    endFlag = false;
                }
            }
        }

        // for(i=1; i<sizeOfPlane-1; i++) {
        //     for(j=((i+1)%2)+1; j<sizeOfPlane-1; j+=2) {
        //         pVal = plane[i][j];
        //         plane[i][j] = (plane[i-1][j] + plane[i+1][j] + plane[i][j-1] + plane[i][j+1])/4;
        //         if(endFlag && tolerance < fabs(plane[i][j]-pVal)) {
        //             endFlag = false;
        //         }
        //     }
        // }

        if(endFlag)
            break;
    }

    return iterations;
}

int main(int argc, char **argv)
{
    unsigned int sizeOfPlane = 10;
    double tolerance = 0.00001;
    double left = 4;
    double right = 2;
    double top = 1;
    double bottom = 3;

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
            case '?':
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
    }

    // for(int i=0; i<6; i++) {
    //     if(argsSet[i]) {
    //         fprintf (stderr, "All arguments must be set\n");
    //         // TODO print help stuff
    //         printf("TODO help info\n");
    //         return 1;
    //     }
    // }

    // Size of the plane must be at least 3x3
    if(sizeOfPlane < 3) {
        fprintf (stderr, "The size of the plane must be greater than 2\n");
        return 1;
    }
    // Tolerance must be greater than 0, or ends with exit code 1
    if(tolerance < 0) {
        fprintf (stderr, "The tolerance must be greater than 0\n");
        return 1;
    }

    plane = newPlane(sizeOfPlane);
    plane = populatePlane(plane, sizeOfPlane, left, right, top, bottom);

    clock_gettime(CLOCK_MONOTONIC, &start);
    iterations = relaxPlane(plane, sizeOfPlane, tolerance);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if(debug)
        printPlane(plane, sizeOfPlane);

    printf("Threads: 1\n");
    printf("Size of Pane: %d\n", sizeOfPlane);
    printf("Iterations: %lu\n", iterations);
    printf("Time: %Lfs\n", toSeconds(start, end));
    return 0;
}