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

double** newPlane2(unsigned int n) {
    double** plane  = ( double** )malloc(n * sizeof(double*));
    plane[0] = ( double * )malloc(n * n * sizeof(double));
 
    for(unsigned int i = 0; i < n; i++)
        plane[i] = (*plane + n * i);

    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            plane[i][j] = 0;
        }
    }

    return plane;
}

double** newPlane1(int n) {
    double** plane = ( double** )malloc(((unsigned int) n) * sizeof(double*));
    for (int i = 0; i < n; ++i)
        plane[i] = ( double* )malloc(((unsigned int) n) * sizeof(double));

    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            plane[i][j] = 0;
        }
    }

    return plane;
}

int main(int argc, char **argv)
{
    int sizeOfPlane = 500;
    int iteration_count = 100000; 
    double** plane1;
    double** plane2;
    struct timespec start, end;

    plane1 = newPlane1(sizeOfPlane);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int k=0; k<iteration_count; k++){
        for(int i=0; i<sizeOfPlane; i++) {
            for(int j=0; j<sizeOfPlane; j++) {
                plane1[i][j] = plane1[i][j] + j + i;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("Plane1: %Lfs\n", toSeconds(start, end));

    plane2 = newPlane2(sizeOfPlane);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int k=0; k<iteration_count; k++){
        for(int i=0; i<sizeOfPlane; i++) {
            for(int j=0; j<sizeOfPlane; j++) {
                plane2[i][j] = plane2[i][j] + j + i;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("Plane2: %Lfs\n", toSeconds(start, end));

    return 0;
}