#### Course: Operating Systems Design (198:518)

#### Project Description

This is a project in thread-safe virtual memory management. The goal is to create a user-level page table from scratch that translates virtual addresses to physical addresses. Here, a 2-layer multi-level page table structure is used to achieve this. It is also supported by a translation lookaside buffer (TLB) cache to reduce translation costs. 


The code can be found here. All relevant functions are packaged as a library as described in the writeup. They are prototyped in the `my_vm.h` file and their implementations can be found in the corresponding `my_vm.c` file.

#### Compilation
- compiling the library: `make clean && make`
- compiling the tests: `cd benchmark && make clean && make`

#### Testing 
This library is benchmarked against a square matrix-matrix multiplication operation. Within the benchmark directory, A single-threaded test can be executed by running `test.c`. The multi-threaded test can be executed by running `multi-test.c`. Running these tests will demonstrate that the library arrives at the correct product deterministically along with metrics for TLB performance. 

To run the tests: 
- single-threaded: `cd benchmark && ./test`
- multi-threaded: `cd benchmark && ./mtest`

#### Further Context 

Along with the technical paper, the `docs` directory also contains the original project specification, if further context is required.
