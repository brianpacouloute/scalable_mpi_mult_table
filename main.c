/*
Winter 2025

Contributors:
    Brian Pacouloute,
    Ethan Chernitzky,
    Thomas De Sa,
    Ammar Ogeil

Overview:
    Basically what we're doing here is giving each process a subset of rows if N = 10 and 4 processes process 0 might get 1-2 etc.
    Each process computes the multiplication products in its rows. Now after this each process sends the product to the process that owns it.
    To calculate who owns what product we use a modular operation:

    owner = product % size;
    ex:
    product = 42; size = 4;
    owner = 42 % 4  = 2

    so 42 is sent to process 2
    each process receives a set of owned numbers and now the counting is done in the processes
    */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <string.h>

// Constants
#define DEFAULT_TABLE_SIZE 100

// Function to check if a number is already in the array
bool isUnique(int *arr, int size, int num)
{
    for (int i = 0; i < size; i++)
    {
        if (arr[i] == num)
            return false;
    }
    return true;
}

/*
    function to record algorithm execution data and save it to a csv file

    int p          - Number of processors
    int N          - Size of multiplication table
    float exe_time - time taken for algorithm to fully run
*/
void save_stats(int p, int N, float exe_time)
{

    // system("mkdir -p output");
    // char *filename = "./output/full_matrix_stats.csv";

    // FILE *file = fopen(filename, "a");
    // if (file == NULL)
    // {
    //     perror("Unable to open file");
    // }
    // fprintf(file, "%d, %d, %f\n", p, N, exe_time); 

    // fclose(file);

    //echoing output to console for SLURM
    printf("%d, %d, %f\n", p, N, exe_time); 
    return; 
}

int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Benchmarking
    double start_time, end_time;
    start_time = MPI_Wtime();

    // change to command line table_size input if none given use constant
    int table_size = 0;
    if (argc != 2)
    {
        if (rank == 0)
            printf("Usage: %s <table_size> : Using default table size 1000\n", argv[0]);
        table_size = DEFAULT_TABLE_SIZE;
    }
    else
    {
        table_size = atoi(argv[1]);
    }

    MPI_Bcast(&table_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int rowsPerProc = table_size / size;
    int start = rank * rowsPerProc + 1;
    int end = (rank == size - 1) ? table_size : start + rowsPerProc - 1;

    // Allocate and prepare buffers for sending to each process
    int **sendBuffers = malloc(size * sizeof(int *));
    int *sendCounts = calloc(size, sizeof(int));
    int *sendCapacities = malloc(size * sizeof(int));

    for (int i = 0; i < size; i++)
    {
        sendCapacities[i] = 100;
        sendBuffers[i] = malloc(sendCapacities[i] * sizeof(int));
    }

    for (int i = start; i <= end; i++)
    {
        for (int j = i; j <= table_size; j++)
        {
            int product = i * j;
            int owner = product % size;
            if (sendCounts[owner] == sendCapacities[owner])
            {
                sendCapacities[owner] *= 2;
                sendBuffers[owner] = realloc(sendBuffers[owner], sendCapacities[owner] * sizeof(int));
                if (!sendBuffers[owner])
                {
                    fprintf(stderr, "Realloc failed on process %d\n", rank);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
            sendBuffers[owner][sendCounts[owner]++] = product;
        }
    }

    // Exchange send counts
    int *recvCounts = malloc(size * sizeof(int));
    MPI_Alltoall(sendCounts, 1, MPI_INT, recvCounts, 1, MPI_INT, MPI_COMM_WORLD);

    // Calculate displacements
    int *sendDispls = malloc(size * sizeof(int));
    int *recvDispls = malloc(size * sizeof(int));
    int totalSend = 0, totalRecv = 0;

    for (int i = 0; i < size; i++)
    {
        sendDispls[i] = totalSend;
        recvDispls[i] = totalRecv;
        totalSend += sendCounts[i];
        totalRecv += recvCounts[i];
    }

    // Flatten send buffers
    int *sendBuffer = malloc(totalSend * sizeof(int));
    int pos = 0;
    for (int i = 0; i < size; i++)
    {
        for (int j = i; j < sendCounts[i]; j++)
        {
            sendBuffer[pos++] = sendBuffers[i][j];
        }
        free(sendBuffers[i]);
    }
    free(sendBuffers);
    free(sendCapacities);

    int *recvBuffer = malloc(totalRecv * sizeof(int));

    // All-to-all communication
    MPI_Alltoallv(sendBuffer, sendCounts, sendDispls, MPI_INT,
                  recvBuffer, recvCounts, recvDispls, MPI_INT,
                  MPI_COMM_WORLD);

    // Now it's safe to free sendCounts
    free(sendCounts);
    free(sendBuffer);
    free(sendDispls);
    free(recvDispls);

    // Deduplicate
    int *localUnique = malloc(100 * sizeof(int));
    int localCount = 0, localCapacity = 100;

    for (int i = 0; i < totalRecv; i++)
    {
        int product = recvBuffer[i];
        if (isUnique(localUnique, localCount, product))
        {
            if (localCount == localCapacity)
            {
                localCapacity *= 2;
                localUnique = realloc(localUnique, localCapacity * sizeof(int));
                if (!localUnique)
                {
                    fprintf(stderr, "Realloc failed in dedup on process %d\n", rank);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
            localUnique[localCount++] = product;
        }
    }

    free(recvBuffer);

    // Reduce total unique count
    int globalCount;
    MPI_Reduce(&localCount, &globalCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // benchmarking
    end_time = MPI_Wtime();

    if (rank == 0)
    {
        // printf("Total unique elements in %dx%d multiplication table: %d\n", table_size, table_size, globalCount);
    }

    free(localUnique);
    free(recvCounts);

    MPI_Finalize();

    if (rank == 0)
    {
        save_stats(size, table_size, end_time-start_time);
    }

    return 0;
}