#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_SIZE 20000003  // reduce collisions

typedef struct Node {
    long long val;
    struct Node *next;
} Node;

typedef struct HashSet {
    Node **buckets;
} HashSet;

unsigned long hash(long long x) {
    return x % HASH_TABLE_SIZE;
}

HashSet *create_hashset() {
    HashSet *set = malloc(sizeof(HashSet));
    set->buckets = calloc(HASH_TABLE_SIZE, sizeof(Node *));
    return set;
}

int insert(HashSet *set, long long val) {
    unsigned long h = hash(val);
    Node *cur = set->buckets[h];
    while (cur) {
        if (cur->val == val) return 0;  // check if it already exists
        cur = cur->next;
    }
    Node *new_node = malloc(sizeof(Node));
    new_node->val = val;
    new_node->next = set->buckets[h];
    set->buckets[h] = new_node;
    return 1;
}

int extract_values(HashSet *set, long long **out_array) {
    int count = 0;
    int capacity = 100000;
    *out_array = malloc(capacity * sizeof(long long));

    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        Node *cur = set->buckets[i];
        while (cur) {
            if (count == capacity) {
                capacity *= 2;
                *out_array = realloc(*out_array, capacity * sizeof(long long));
            }
            (*out_array)[count++] = cur->val;
            cur = cur->next;
        }
    }
    return count;
}

void destroy_hashset(HashSet *set) {
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        Node *cur = set->buckets[i];
        while (cur) {
            Node *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(set->buckets);
    free(set);
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

    // row distribution
    int rowsPerProc = N / size;
    int extra = N % size;
    int start = rank * rowsPerProc + (rank < extra ? rank : extra) + 1;
    int end = start + rowsPerProc - 1 + (rank < extra ? 1 : 0);

    HashSet *local_set = create_hashset();
    for (int i = start; i <= end; ++i) {
        for (int j = 1; j <= N; ++j) {
            insert(local_set, (long long)i * j);
        }
    }

    long long *local_values = NULL;
    int local_count = extract_values(local_set, &local_values);
    destroy_hashset(local_set);

    if (rank == 0) {
        HashSet *global_set = create_hashset();
        for (int i = 0; i < local_count; i++) {
            insert(global_set, local_values[i]);
        }
        free(local_values);

        for (int src = 1; src < size; ++src) {
            int recv_count;
            MPI_Recv(&recv_count, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            long long *recv_buf = malloc(recv_count * sizeof(long long));
            MPI_Recv(recv_buf, recv_count, MPI_LONG_LONG, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < recv_count; ++i) {
                insert(global_set, recv_buf[i]);
            }
            free(recv_buf);
        }

        long long *global_values;
        int total_unique = extract_values(global_set, &global_values);
        printf("Total unique elements in %dx%d multiplication table: %d\n", N, N, total_unique);
        free(global_values);
        destroy_hashset(global_set);
    } else {
        MPI_Send(&local_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(local_values, local_count, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
        free(local_values);
    }

    MPI_Finalize();
    return 0;
}
