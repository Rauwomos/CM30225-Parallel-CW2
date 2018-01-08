# CM30225-Parallel-CW1
3rd Year Parallel Coursework 2

To compile the files run the follow commands:
  ```shell
  mpicc -Wall -Werror -Wextra -Wconversion -Wpedantic -lrt -std=gnu11 mpi.c -o mpi.out

  gcc -Wall -Werror -Wextra -Wconversion -Wpedantic -lrt -std=gnu11 single.c -o single.out
  ```

To run the programs you must pass in arguments for the values of each edge, the size of the array, the precision to work to and if needed the number of threads to use:
  ```shell
  ./single.out -s 1000

  mpirun -n 8 mpi.out -s 1000
  ```
  ```
  -u give value for top edge
  -d give value for bottom edge
  -r give value for right edge
  -l give value for left edge
  -p give precision to work to
  -s give size of the array. e.g 100 gives 100x100 array
  ```