#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int i,j,sizeOfPlane;
    sizeOfPlane = 5;
    for(i=1; i<sizeOfPlane-1; i++) {
            for(j=(i%2)+1; j<sizeOfPlane-1; j+=2) {
                printf("i: %d j: %d\n", i,j);
            }
        }

        for(i=1; i<sizeOfPlane-1; i++) {
            for(j=((i+1)%2)+1; j<sizeOfPlane-1; j+=2) {
                printf("i: %d j: %d\n", i,j);
            }
        }

    return 0;
}