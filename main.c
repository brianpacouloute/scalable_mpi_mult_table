
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_SIZE 20000003

typedef struct Node {
    long long val;
    struct Node* next;
} Node;

typedef struct HashSet {
    Node** buckets;
} HashSet;

unsigned long safe_mod(long long x, int mod) {
    return (x % mod + mod) % mod;
}

unsigned long hash(long long x) {
    return (unsigned long)(x % HASH_TABLE_SIZE);
}

HashSet* create_set() {
    HashSet* set = malloc(sizeof(HashSet));
    set->buckets = calloc(HASH_TABLE_SIZE, sizeof(Node*));
    return set;
}

int insert(HashSet* set, long long val) {
    unsigned long h = hash(val);
    Node* curr = set->buckets[h];
    while (curr) {
        if (curr->val == val) return 0;
        curr = curr->next;
    }
    Node* node = malloc(sizeof(Node));
    node->val = val;
    node->next = set->buckets[h];
    set->buckets[h] = node;
    return 1;
}

void free_set(HashSet* set) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Node* curr = set->buckets[i];
        while (curr) {
            Node* tmp = curr;
            curr = curr->next;
            free(tmp);
        }
    }
    free(set->buckets);
    free(set);
}

int main(int argc, char* argv[]) {
    int rank, size, N;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Benchmarking
    double start_time, end_time;
    start_time = MPI_Wtime();

    if (argc < 2) {
        if (rank == 0)
            printf("Usage: %s N\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    N = atoi(argv[1]);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int base = N / size;
    int extra = N % size;
    int start = rank * base + (rank < extra ? rank : extra) + 1;
    int rows = base + (rank < extra ? 1 : 0);
    int end = start + rows - 1;

    // Count how many products will go to each rank
    int* send_counts = calloc(size, sizeof(int));
    for (int i = start; i <= end; i++) {
        for (int j = 1; j <= i; j++) {
            long long prod = (long long)i * j;
            int dest = safe_mod(prod, size);
            send_counts[dest]++;
        }
    }

    // Compute send displacements and total send size
    int* send_displs = calloc(size, sizeof(int));
    int total_send = send_counts[0];
    for (int i = 1; i < size; i++) {
        send_displs[i] = send_displs[i - 1] + send_counts[i - 1];
        total_send += send_counts[i];
    }

    long long* send_buf = malloc(total_send * sizeof(long long));
    int* send_offsets = calloc(size, sizeof(int));  // for filling send_buf

    for (int i = start; i <= end; i++) {
        for (int j = 1; j <= i; j++) {
            long long prod = (long long)i * j;
            int dest = safe_mod(prod, size);
            int pos = send_displs[dest] + send_offsets[dest];
            send_buf[pos] = prod;
            send_offsets[dest]++;
        }
    }

    // Exchange counts
    int* recv_counts = calloc(size, sizeof(int));
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    // Compute recv displacements and total receive size
    int* recv_displs = calloc(size, sizeof(int));
    int total_recv = recv_counts[0];
    for (int i = 1; i < size; i++) {
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
        total_recv += recv_counts[i];
    }

    long long* recv_buf = malloc((total_recv > 0 ? total_recv : 1) * sizeof(long long));  // never NULL

    // Alltoallv
    MPI_Alltoallv(
        send_buf, send_counts, send_displs, MPI_LONG_LONG,
        recv_buf, recv_counts, recv_displs, MPI_LONG_LONG,
        MPI_COMM_WORLD
    );

    // Deduplicate
    HashSet* set = create_set();
    for (int i = 0; i < total_recv; i++)
        insert(set, recv_buf[i]);

    int local_unique = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Node* curr = set->buckets[i];
        while (curr) {
            local_unique++;
            curr = curr->next;
        }
    }

    int global_unique = 0;
    MPI_Reduce(&local_unique, &global_unique, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    //benchmarking
    end_time = MPI_Wtime(); 
    if (rank == 0)
        printf("%d, %d, %f, %d\n", size, N, end_time-start_time, global_unique); 
        // printf("Total unique elements in %dx%d multiplication table: %d\n", N, N, global_unique);

    // Cleanup
    free(send_counts); free(send_displs); free(send_offsets);
    free(recv_counts); free(recv_displs); free(send_buf); free(recv_buf);
    free_set(set);

    MPI_Finalize();

    return 0;
}
