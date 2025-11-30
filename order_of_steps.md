############
## PART 1 ##
############

1. set bit values for the relevant variables and associated bitmasks:


```c
// -----------------------------------------------------------------------------
//  Memory and Paging Configuration
// -----------------------------------------------------------------------------

#define VA_BITS        32u           // Simulated virtual address width
#define PGSIZE         4096u         // Page size = 4 KB

#define MAX_MEMSIZE    (1ULL << 32)  // Max virtual memory = 4 GB
#define MEMSIZE        (1ULL << 30)  // Simulated physical memory = 1 GB

#define OFFSET_BITS    log2(PGSIZE);
#define PDX_BITS       (32 - OFFSET_BITS) / 2
#define PTX_BITS       (32 - OFFSET_BITS) - PDX_BITS

// --- Constants for bit shifts and masks ---
#define PDXSHIFT       PTXBITS + OFFSET_BITS    /** TODO: number of bits to shift for directory index **/
#define PTXSHIFT       OFFSET_BITS              /** TODO: number of bits to shift for table index **/
#define PXMASK         (1 << PTX_BITS) - 1      /** TODO:  **/
#define OFFMASK        (1 << OFFSET_BITS)  - 1  /** TODO:  **/

// --- Macros to extract address components ---
#define PDX(va)       /** TODO: compute directory index from virtual address **/
#define PTX(va)       /** TODO: compute table index from virtual address **/
#define OFF(va)       /** TODO: compute page offset from virtual address **/
```

2. allocate physical memory using `mmap`

```c
// TODO: why use mmap over malloc?
static void* physical_memory;

/*
 * set_physical_mem()
 * ------------------
 * Allocates and initializes simulated physical memory and any required
 * data structures (e.g., bitmaps for tracking page use).
 *
 * Return value: None.
 * Errors should be handled internally (e.g., failed allocation).
 */
void set_physical_mem(void) {
  // implement memory allocation for simulated physical memory.
  // use 32-bit values for sizes, page counts, and offsets.
  
  // https://man7.org/linux/man-pages/man2/mmap.2.html
  // TODO: check if these are the correct parameters for this function call

  physical_memory = mmap(null,
                         MEMSIZE,
                         prot_read | prot_write,
                         map_private | map_anonymous,
                         -1,
                         0);

  if(physical_mem == map_failed) {
    perror("mmap failed.");
    exit(1);
  }
}
```

3. generate the macros to isolate each segment of the VA:

```c
#define PDX(va)        va  >> (OFFSET_BITS + PTX_BITS)              /** TODO: compute directory index from virtual address **/
#define PTX(va)        (va >> OFFSET_BITS) & ((1 << PTX_BITS) - 1)  /** TODO: compute table index from virtual address **/
#define OFF(va)        va  >> (1 << OFFSET_BITS) - 1)               /** TODO: compute page offset from virtual address **/
```

4. set up helper functions for virtual and physical bitmaps
```c
// bitmap getters/setters
void set_bit(char* bmap, int idx) { 
  uint32_t target_byte = bmap[idx / 8];
  uint8_t target_bit = idx % 8;
  target_byte |= (1 << (target_bit));
}

int get_bit(char* bmap, int idx) {
  uint8_t target_byte = bmap[idx / 8];
  uint8_t target_bit  = idx % 8;
  return target_byte & (1 << (target_bit);
}

void clear_bit(char* bmap, int idx) {
  uint8_t target_byte = bmap[idx / 8];
  uint8_t target_bit  = idx % 8;
  return target_byte &= ~(1 << (target_bit);
}
```

5. setup page directory within the buffer (top n frames based on PGSIZE)

    ```c
    // the top frame(s) are reserved for the page directory
    uint32_t max_pd_entries = 1 << PDX_BITS;
    uint32_t max_pd_bytes = max_pd_entries * sizeof(pde_t);
    uint32_t max_pd_pages = (max_pd_bytes + PGSIZE - 1) / PGSIZE;

    pde_t* pg_dir_top = (pde_t*)p_buff;
    memset(page_dir_top, 0, max_pd_bytes);

    // mark frames are occupied in the physical bitmap
    // note: the virtual bitmap only maintains the `n_malloc`d blocks
    uint32_t max_frames_in_bytes = max_pd_bytes / 8;
    memset(p_bmap, 0xFF, max_frames_in_bytes);

    uint32_t remainder_frames = max_pd_bytes % 8;
    for(int i = 0; i < remainder_frames; i++) {
      set_bit(p_bmap, (max_frames_in_bytes * 8) + i);
    }
     
    ```

    - note: explicitly zero out the pgdir bytes. this ensures that an IN_USE flag can be
    set explicitly for each pde_t entry (LSB).

6. `translate()`: implementation

DESIGN DISCUSSION: discuss why you chose 10bit for each level of the multi-level page table. 
    - page tables entries correspond to 12 bit offsets (ie. 1 page per entry)
    - page tables must be fully allocated (static arrays) upon initialization
        - pro: pointer arithmetic for fast updates/lookups
        - con: a very page table would require more memory during allocation causing internal fragmentation
            - linear page table takes a huge amount of memory in one go!
            - it needs an entry for every virtual page in process address space, even if those pages are empty
            - spreading the bits facilitates the multi-level page queue part of this structure
            - MLPQs keep the page tables small and modular


   ```c

   pte_t* translate(pde_t* pgdir, void* va)
   {
       // TODO: Extract the 32-bit virtual address and compute indices
       // for the page directory, page table, and offset.
       // Return the corresponding PTE if found.
    
       vaddr32_t v_addr = VA2U(va);

       uint32_t pgdir_idx = PDX(v_addr);
       pde_t pgdir_entry = pgdir[pgdir_idx];
       if(pgdir_entry == 0) return NULL;

       uint32_t pgtbl_idx = PTX(v_addr);
       uint32_t* pgtbl = (pte_t*)(pgdir_entry & ~OFFMASK);
       pte_t* pgtbl_entry = &(pgtbl[pgtbl_idx]);
       if(pgtbl_entry == 0) return NULL;

       return pgtbl_entry;
   }
   ```

7. `map_page`: implementation
    - objective: create a virtual to physical address mapping
    - input: pde_t* pgdir, VA, PA

    - extract the PDX, PTX indices from the VA
    - check if the target page table exists using PDX => pgtbl_ptr
        - if false: 
            - allocate a new page table in the buffer (1 frame)
            - zero the frame 
            - store the pde_t[frame address + IN_USE flag] at pgdir[PDX]

    - check if pgtbl[PTX] exists
        - if true:
            - the virtual address is already mapped to a frame
            - return error "invalid va."

        - if false:
            - set pgtbl[PTX] = pa | IS_ALLOCD

    ```c

    int map_page(pde_t *pgdir, void *va, void *pa)
    {
        // TODO: map virtual address to physical address in the page tables.
        // make this threadsafe
        // check if there is an existing mapping for a virtual address
        vaddr32_t v_addr = VA2U(va);
        uint32_t pgdir_idx = PDX(v_addr);
        uint32_t pgtbl_idx = PTX(v_addr);
      
        // "upsert" the target page table
        if(!(pgdir[pgdir_idx] & IN_USE)) {
          // allocate pgtbl and zero it
          void* pgtbl_frame = alloc_frame();
          if(pgtbl_frame == 0) return -1;
      
          memset(pgtbl_frame, 0, PGSIZE);
          pgdir[pgdir_idx] = (uint32_t)(uintptr_t)pgtbl_frame | IN_USE;
        }

        pte_t* pgtbl = (pte_t*)(pgdir[pgdir_idx] & ~OFFMASK);
        if(!(pgtbl[pgtbl_idx] & IN_USE)) {
          pgtbl[pgtbl_idx] = (uint32_t)(uintptr_t)pa | IN_USE;
          return 0;
        }

        return -1;
    }


    ```

- some other steps that were done here:
    - `IN_USE` flag: marks the pde_t and pte_t entries if already being used, through LSB.
        - note: pde_t and pte_t entries are frame-addressable so lower 12 bits are always 0 (1GB buffer)
        - the only way for the LSB to be 1 is because the frame already allocated.

    - `pgdir` is now a static global variable 
    - `lock` pthread mutex is initialized


8. `void* get_next_avail(int num_pages)`:
    - using first fit policy, find the index of the first substring of 0s that fits num_pages

    ```c

    void *get_next_avail(int num_pages)
    {
        if(num_pages <= 0) return NULL;

        uint32_t chunk_start = 0; 
        uint32_t ctr = 0;

        for(int i = 0; i < (MAX_MEMSIZE / PGSIZE) ; i++) {
          if(get_bit(v_bmap, i) == 0) {
            ctr++;
            if(ctr == num_pages) {
              // return void* to the corresponding vpage addr
              uint32_t vpage_byte_offset = chunk_start * PGSIZE;
              return U2VA(vpage_byte_offset);
            }
          } else {
            chunk_start = i+1; 
            ctr = 0;
          }
        }

        return NULL; // No available block placeholder.
    }

    ```

    - the chunk_start value represents the idx of the v_page (in 4GB space)
    - 

9. `n_malloc`:

    ```c

    void *n_malloc(unsigned int num_bytes)
    {
        if(pgdir == NULL) set_physical_mem();

        uint32_t num_pages = (num_bytes + PGSIZE - 1) / PGSIZE;
        vaddr32_t va_base = VA2U(get_next_avail(num_pages));
        if(!va_base) return NULL;

        for(uint32_t i = 0; i < num_pages; i++) {
          void* pa = alloc_frame();
          if(pa == NULL) return NULL;

          void* va = U2VA(va_base + (i * PGSIZE));
          int ret = map_page(pgdir, va, pa);
          if(ret == -1) return NULL; 

          
          uint32_t v_page_bit_idx = (va_base / PGSIZE) + i;
          set_bit(v_bmap, v_page_bit_idx);
        }

        return U2VA(va_base);
    }
    ```

10. `free`:
  // TODO: Clear page table entries, update bitmaps, and invalidate TLB.
  // 1. take a virtual address and size
  // 2. calculate how many pages to free: size to PGSIZE units (rounded up)
  // 3. for each page:
  //  - skip if page is not actually allocated (check FLAG)
  //  - translate va -> pa
  //  - clear p_bmap and v_bmap bits
  //  - clear pgtbl_entry

    ```c

    void n_free(void *va, int size)
    {
      if(va == NULL || size <= 0) return;

      vaddr32_t va_base = VA2U(va);
      uint32_t num_pages = (size + PGSIZE - 1) / PGSIZE;

      for(uint32_t i = 0; i < num_pages; i++) {
        vaddr32_t va = va_base + (i * PGSIZE);
        pte_t* pte = translate(pgdir, U2VA(va));
        
        // cannot free unallocated frame
        if(pte == NULL || ((*pte & IN_USE) == 0)) continue; 
        
        // clear corresponding v_page bit
        uint32_t v_page_bit_idx = va / PGSIZE;
        clear_bit(v_bmap, v_page_bit_idx);

        // clear corresponding p_frame bit
        paddr32_t pa = (*pte & ~OFFMASK);
        uint32_t p_frame_bit_idx = (pa - (paddr32_t)(uintptr_t)p_buff) / PGSIZE;
        clear_bit(p_bmap, p_frame_bit_idx);
        
        // clear corresponding pgtbl entry
        *pte = 0;
      }
    }
    ```

11. 

############
## PART 2 ##
############
