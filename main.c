# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>
# include <mpi.h>

# define MAX_ELEMENTS 100000

//  this will be used to see if a number is already in the list
bool is_unique(int *uniqueList, int size, int num) {
    for (int i = 0; i < size; i++) {
        if (uniqueList[i] == num) {
            return false;
        }
    }
    return true;
}

int merge_unique (int *dest, int *src, int dest_size, int src_size) {
    for (int i = 0; i < src_size; i++) {
        if (is_unique(dest, dest_size,src[i])) {
            dest[dest_size++] = src[i];
        }
    }
    return dest_size;
}

// function to count unique elements in the multiplication table
int count_unique_elements(int N) {
    int *unique_list = (int *) malloc(N * N * sizeof(int)); // store unique elements
    int unique_count = 0; // count of unique elements

    // generate multiplication table
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            int product = i * j;
            if (is_unique(unique_list, unique_count, product)) {
                unique_list[unique_count++] = product;
            }
        }
    }

    free(unique_list);
    return unique_count;
}

int main(int arg_count, char *arg_values[]) {
    int rank, size, N;

    MPI_Init(&arg_count, &arg_values);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    // Temporary N until i switch to command line argument N
    if (rank == 0) {
        printf("Enter the value of N: ");
        fflush(stdout);
        scanf("%d", &N);
    }

    // broadcast N to all processors
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // computer row range for this process
    int rows_per_proc = N / size;
    int start = rank * rows_per_proc + 1;
    int end = (rank == size - 1) ? N : start + rows_per_proc - 1;

    int local_unique[MAX_ELEMENTS];
    int local_count = 0;

    // build local unique sets
    

    printf("The number of unique elements in the multiplication table of size %d is %d\n", N, count_unique_elements(N));
    return 0;
}