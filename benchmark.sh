#!/bin/bash
#SBATCH --job-name=mpi_multiplication_test
#SBATCH --output=test.txt
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=40
#SBATCH --ntasks-per-socket=20
#SBATCH --partition=compute

# Load modules
module purge
module load StdEnv/2023 gcc/12.3 openmpi/4.1.5

array=(50000)
for procs in {1..40..1}
do
  for N in "${array[@]}"
  do
    srun -n $procs ./main.out $N  >> test.txt
  done
done
