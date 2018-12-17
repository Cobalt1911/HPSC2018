#!/bin/bash
#
module load openmpi/1.8.4/gcc-4.9.2
#
#SBATCH --job-name=test_mpi
#SBATCH --mail-user=cmoureau@student.uliege.be
#SBATCH --mail-type=ALL
#SBATCH --output=scalability_weak_expl.txt
#
#SBATCH --ntasks=8
#SBATCH --cpus-per-task=1
#      ####################  maximum number of cores per node on this cluster
#                            to be sure to be alone on the node

#####  #SBATCH --exclusive
#####  #      ############# OR --exclusive to get a full node for the job

#SBATCH --time=60:00
#SBATCH --mem-per-cpu=3000


#mpicc -g -o Octopus octopus.c explicit.c implicit.c COO_CSR_BSR.c fileIO.c algorithms.c -O0 -lm -std=c99 -fopenmp

echo "Explicit";
OMP_NUM_THREADS=8

echo "L1";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 1 ./Octopus param_big.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."

echo "L2";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 2 ./Octopus param_big2.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."

echo "L3";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 3 ./Octopus param_big3.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."

echo "L4";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 4 ./Octopus param_big4.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."

echo "L5";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 5 ./Octopus param_big5.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."

echo "L6";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 6 ./Octopus param_big6.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."

echo "L7";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 7 ./Octopus param_big7.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."

echo "L8";
t0=$(date +%s%3N)
mpiexec --bind-to none -np 8 ./Octopus param_big8.dat 0;
t1=$(date +%s%3N)
echo "Elapsed wall clock time = " $(( $t1-$t0 )) " milliseconds."
