clear
mpicc -g -o Main explicitCSR.c -lm -std=c99 -fopenmp
mpiexec -np 4 time ./Main param_tiny.dat

#mpicc -g -o Main explicit.c -lm -std=c99 -fopenmp
#mpiexec -np 4 time ./Main param_tiny.dat

#gcc -o Main CSR_BSR_example.c -lm -std=c99
#time ./Main
