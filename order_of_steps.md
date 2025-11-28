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





############
## PART 2 ##
############
