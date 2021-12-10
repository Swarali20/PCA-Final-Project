# Parallel Page Rank

## Authors 

Aditya Bhawkar and Swarali Patil

## About the project 

This repository contains all the code for various incremental versions for calculating Page Rank. We have used C++, aided by OpenMP, MPI and SIMD to achieve speedup on multicore processors. 

## Code 

`original` contains the code implemented using STL Maps in C++, which gives us very poor performance due to slow lookup during main iteration. 

`original_with_vectors` is an optimized version of the baseline using STL vectors instead of maps, which take advantage of contiguous memory allocation. `optimizied_original_with_vectors` is a slightly faster version of the same.

`simple_openmp`, `simple_simd` `openmp_with_simd` are our first attempts at parallelism where we perform page rank in parallel. 

`mpi` contains the Message Passing Interface version

## To run: 

Each directory has it's own MakeFile
To compile, simply run

<code>make</code>

To run the compiled code

<code>./serial -f ../input_graphs/graph.txt </code>

<code>./parallel -f ../input_graphs/graph.txt -n number of threads </code>
  
<code> mpirun -np number of processors ./parallel -f ../input_graphs/graph.txt </code>  
  

  
