#!/bin/bash
#PBS -l nodes=4:ppn=2
#PBS -l walltime=00:0:10
#PBS -o pfilter.out
#PBS -A lc_an2
#PBS -j oe
#PBS -N pfilter


filter=median

# note that we will vary window size when testing
window_size=4
mpirun -np 4 ./pfilter input/moon_in.tiff moon_out.tiff $filter $window_size

