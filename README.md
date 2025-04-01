
# Parallel Unique Product Counter using MPI

This project implements a **fully parallel algorithm** using **MPI** to count the number of **unique products in an NxN multiplication table**, even for extremely large values of N (up to 100,000), where over **10 billion products** are evaluated in under **3 minutes**.

The algorithm uses a **hash-based ownership strategy** and a **custom hash set implementation** to ensure perfect deduplication and workload balance across all processes.

---

## Overview

Given an NxN multiplication table, this program calculates how many **distinct values** appear in the entire table.

### Why this problem is interesting:

- A naive serial algorithm becomes infeasible for large N due to **memory** and **runtime** constraints.
- Many products in the table are repeated (e.g., 3×4 and 4×3), so deduplication is non-trivial.
- We scale this problem using **distributed computing and MPI**, reaching **billions of products** in minutes.

---

## Key Features

- Scales to N = 100,000 (10 billion products)
- Linear speedup with more processes
- Uses `MPI_Alltoallv` to distribute products across ranks
- Each process owns a slice of the product space via a **destination hashing function**
- Deduplication handled with a **custom-built hash set** using **separate chaining**
- Fully written in **C**

---

## How It Works

1. **Row Partitioning**: Each process is assigned a subset of the table's rows to compute `i × j` products.
2. **Destination Assignment**: Each product is hashed to determine which process should "own" it.
3. **MPI Communication**: Products are exchanged using `MPI_Alltoallv`, so each rank receives only the products it is responsible for.
4. **Deduplication**: Each rank inserts its received products into a custom hash set to filter out duplicates.
5. **Global Aggregation**: `MPI_Reduce` is used to sum all local unique counts into a final global result.

---

## How to Run

### Prerequisites

- C compiler with MPI (e.g., `mpicc`)
- A system with MPI installed (e.g., OpenMPI or MPICH)

### Compilation

```bash
mpicc main.c -o unique_counter.out
