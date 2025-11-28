## part 1

- main motive: move the page table and memory allocation to software
- create a user-level page table 
    - translate virtual addresses to physical addresses
    - use a multi-level page table structure

- test your implementation 
    - use different page sizes
    - evaluate performance. focus on: 
        - code correctness 
        - page table accuracy
        - TLB state consistency

#### implement of virtual memory system

- emulate RAM: 
    - allocate a large contiguous region and manage virtual to physical mappings 
    - implement `n_malloc()`: returns a virtual address that maps to a physical page 
    - simulate a 32-bit address space supporting 4 GB of virtual memory.

    - vary the physical memory size and the page size when testing your implementation

- keep the simulation portable: 
    - treat all simulated addresses and page-table fields as 32-bit values 
    - use uint32_t for:
        - virtual addresses 
        - physical addresses 
        - page-directory entries 
        - page-table entries 

    - convert between void* and the 32-bit form only at the API boundary.

- we have provided helper functions to safely convert a void* 
    - ex: function to convert 64-bit pointer on your system to a 32-bit vaddr32_t and back 
    - thus, we can perform address arithmetic using 32-bit values inside your simulation independent of the host architecture

    ```c
        #include <stdint.h>

        typedef uint32_t vaddr32_t; // simulated 32-bit virtual address
        typedef uint32_t paddr32_t; // simulated 32-bit physical address
        typedef uint32_t pte_t;     // page table entry (flags you define)
        typedef uint32_t pde_t;     // page directory entry

        static inline vaddr32_t VA2U(void *va)    { return (vaddr32_t)(uintptr_t)va; }
        static inline     void* U2VA(vaddr32_t u) { return (void*)(uintptr_t)u;      }
    ```
   
- `set_physical_mem()`: 
    - responsibility: allocate memory buffer using `mmap` or `malloc`
        - this creates an illusion of physical memory (coordinating with page table/memory manager)
        - physical memory: refers to a large region of contiguous memory allocated using `mmap()` or `malloc()` 
        - linux `http://man7.org/linux/man-pages/man2/mmap.2.htm`

    - feel free to use `malloc` to allocate other data structures required to manage your virtual memory

- `translate():`
    - before implementing your functions, you must complete several parts of the header file `my_vm.h` 
    - i.e. define how a 32-bit virtual address is divided into:
        - page directory 
        - page table 
        - offset fields 
            - this is done by filling in the bit shifts, masks, and macros used to extract each part

        ```c
        // --- constants for bit shifts and masks ---
        #define PDXSHIFT                                    /** TODO: number of bits to shift for directory index **/         
        ...

        ```

    - workflow:
        - use PDX(va) to find the directory index (which selects the correct page table)
        - use PTX(va) to find the entry within that page table 
        - combine the page’s physical frame number (PFN) with OFF(va) to produce the physical address.

        - next, the translate function:
            - input: the address of the outer page directory (a.k.a the outer page table)
            - input: a virtual address, 
            - returns: corresponding physical address 

        - you have to work with two-level page tables

- ex: 4KB page size configuration:
    - each level uses 10-bits with 12-bits reserved for offset

    - for other page sizes X 
        - reserve log_2(X) bits for offset 
        - split the remaining bits into two levels (unequal division is possible in this case)

- `page_map()`: 
    - responsibility: walks the page directory to check if there is an existing mapping for a VA 
    - if the VA is not present, a new entry will be added 
    - note: use this in `n_malloc()` to add page table entry

- `n_malloc():` 
    - responsibility: takes as input: the bytes to allocate and return: a virtual address 
    - because you are using a 32-bit virtual address space, you are responsible for address management 
    - assume: for each allocation, you will allocate one or more pages (depending on the size of your allocation) 

    - ex: if you call `n_malloc(1 byte)` twice, you will allocate one page for each call. 
    - ex: if you call `n_malloc(4097 bytes)`, you will allocate 2 pages when the page size is set as 4KB.

    - if the user allocates a memory size that is larger than one-page size (e.g., 8KB bytes for 4KB page size): 
        - you will allocate multiple pages 
        - these can be either physically contiguous or non-contiguous in the physical memory depending on the availabilityNote

        - note: that this approach causes internal fragmentation 
            - to allocate 1 byte of memory, we must allocate at least 1 page 
            - for simplicity, you don’t have to handle internal fragmentation in this in this project 
            - (unless you are aiming for extra credits)

        - we describe an approach to reducing internal fragmentation found in Part B of the extra credit section

    - keeping track: 
        - keep track of physical pages that are already allocated or free 
        - to keep track, use a virtual and physical page bitmap that represents a page (you must use a bitmap) 
        - you can use the bitmap functions from Project 1 

    - if you do not use a bitmap, you will lose points 
        - (for example, using an array of chars with ‘y’ or ‘n’ or an array of ints using each index for a page)

    - the get_next_avail() (see the code) function must return the next free available page 
    - you must implement bitmaps efficiently (allocating only one bit per page) to avoid wasting memory
        - https://www.cprogramming.com/tutorial/bitwise_operators.html 
        - see the C Review resources on Canvas for some simple examples

- why use physical and virtual bitmaps? 
    - we use the physical bitmap (1 bit for each page) 
    - quickly find the next available free physical page, which can be present anywhere in the RAM

    - the use of a virtual bitmap is optional 
        - the virtual bitmap expedites finding free page entries

    - alternatively, you can walk the page table/page directories to find free entries. 
    - however, you might need to maintain some extra information within each entry to do this

-`n_free()`: 
  - input:  takes a virtual address 
  - input:  the total bytes (int) 
  - action: releases (frees) pages starting from the page representing the virtual address. 

  - ex: for 4KB pages, `n_free(0x1000, 5000)` will free two pages starting at virtual addresses 0x1000 
  - ensure n_free() isn’t deallocating a page that hasn’t been allocated yet! 

  - note: `n_free` returns success only if all the pages are deallocated


  - you should be able to free non-contiguous physical pages using the information from your page tables
    - the virtual address is given as an argument in `n_free()` otherwise 

    - when a user frees one or more pages, you would have to:
        - update the virtual bitmap
        - update the physical bitmap (marking the corresponding page’s bitmap to 0) 
        - clean the page table entries

- beyond `n_malloc()` and `n_free()`, we need two additional methods that use virtual addresses to store or load data
    - `put_data`
    - `get_data`

- `put_data()`: 
    - args: 
        - a virtual address 
        - a value pointer
        - the size of the value pointer

    - action: directly copies them to physical pages (what does this mean?) 
    - check for validity of library's virtual address
    - function returns 0 if successful, -1 if put_data fails

- `get_data()`:
    - args: same as `put_data()`
    - action: reads data from the virtual address to value buffer
    - if implementing TLB, check the presence of translation in TLB before proceeding

- `mat_mult()`: 
    - args: 
        - two matrices `mat1` and `mat2` 
        - a size argument representing the number of rows and columns 
        - action: reads values from matrices, perform matrix multiplication and copy the result to the answer array 
        - take a look at the test example 

        - for indexing the matrices, use the following method:
            - A[i][j] = A[(i * size_of_rows * value_size) + (j * value_size)]

- important notes: 
    - you cannot change the function arguments/signature of the following: 
        - n_malloc(), n_free(), put_data(), get_data(), mat_mult()
        - your code must be thread-safe and your code will be tested with multi-threaded benchmarks

    - we have included one sample test code (benchmark/multi_test.c) to check your code for thread safety 
    - this is just a sample; feel free to use (by adding to Makefike), extend, increase thread count to verify the correctness

