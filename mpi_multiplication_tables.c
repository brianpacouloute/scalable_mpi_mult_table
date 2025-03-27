
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITSET_SIZE 1000000000
char *bitset;

unsigned long hash1(long long key) { return key % BITSET_SIZE; }
unsigned long hash2(long long key) { return (key / 3) % BITSET_SIZE; }

void bitset_add(long long key) {
    bitset[hash1(key)] = 1;
    bitset[hash2(key)] = 1;
}

int bitset_contains(long long key) {
    return bitset[hash1(key)] && bitset[hash2(key)];
}

int main(int argc, char *argv[]) {
    int rank, size, N;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) printf("Usage: %s N\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    N = atoi(argv[1]);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Balanced row distribution
    int rowsPerProc = N / size;
    int extra = N % size;
    int start = rank * rowsPerProc + (rank < extra ? rank : extra) + 1;
    int end = start + rowsPerProc - 1 + (rank < extra ? 1 : 0);

    // Allocate bitset
    bitset = (char *)calloc(BITSET_SIZE, sizeof(char));
    if (!bitset) {
        fprintf(stderr, "Rank %d: Bitset allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Allocate buffer for unique values
    int capacity = 100000;
    int unique_index = 0;
    long long *unique_values = (long long *)malloc(capacity * sizeof(long long));
    if (!unique_values) {
        fprintf(stderr, "Rank %d: unique_values allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    long long product;
    int local_unique_count = 0;

    // Loop through rows and columns
    for (int i = start; i <= end; ++i) {
        for (int j = 1; j <= N; ++j) {
            product = (long long)i * j;
            if (!bitset_contains(product)) {
                bitset_add(product);

                // Ensure capacity
                if (unique_index == capacity) {
                    capacity *= 2;
                    unique_values = realloc(unique_values, capacity * sizeof(long long));
                    if (!unique_values) {
                        fprintf(stderr, "Rank %d: realloc failed\n", rank);
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }
                }

                unique_values[unique_index++] = product;
                local_unique_count++;
            }
        }
    }

    if (rank == 0) {
        // Receive values from other ranks
        for (int i = 1; i < size; i++) {
            int received_size;
            MPI_Recv(&received_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            long long *recv_values = (long long *)malloc(received_size * sizeof(long long));
            if (!recv_values) {
                fprintf(stderr, "Rank 0: recv buffer allocation failed\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Recv(recv_values, received_size, MPI_LONG_LONG, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int j = 0; j < received_size; j++) {
                if (!bitset_contains(recv_values[j])) {
                    bitset_add(recv_values[j]);
                    local_unique_count++;
                }
            }

            free(recv_values);
        }

        printf("Total unique elements in %dx%d multiplication table: %d\n", N, N, local_unique_count);
    } else {
        // Send unique values to rank 0
        MPI_Send(&unique_index, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(unique_values, unique_index, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
    }

    free(unique_values);
    free(bitset);
    MPI_Finalize();
    return 0;
}