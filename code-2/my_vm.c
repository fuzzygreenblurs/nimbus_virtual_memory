
#include "my_vm.h"
#include <string.h>   // optional for memcpy if you later implement put/get
#include <sys/mman.h>
#include <pthread.h>

// -----------------------------------------------------------------------------
// Global Declarations (optional)
// -----------------------------------------------------------------------------

struct tlb tlb_store; // Placeholder for your TLB structure

// Optional counters for TLB statistics
static unsigned long long tlb_lookups = 0;
static unsigned long long tlb_misses  = 0;

static void* p_buff;
static void* p_bmap;
static void* v_bmap;
  
// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
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
  // TODO: implement memory allocation for simulated physical memory.
  // use 32-bit values for sizes, page counts, and offsets.
  
  // https://man7.org/linux/man-pages/man2/mmap.2.html
  p_buff = mmap(NULL,
               MEMSIZE,
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, 
               -1,
               0);

  if(p_buff == MAP_FAILED) {
    perror("mmap failed.");
    exit(1);
  }

  uint32_t p_bmap_bytes = ((MEMSIZE / PGSIZE) + 7) / 8;
  p_bmap = malloc(p_bmap_bytes);

  uint32_t v_bmap_bytes = ((MAX_MEMSIZE / PGSIZE) + 7) / 8;
  v_bmap = malloc(v_bmap_bytes);

  // the top frame(s) are reserved for the page directory
  uint32_t max_pd_entries = 1 << PDX_BITS;
  uint32_t max_pd_bytes = max_pd_entries * sizeof(pde_t);
  uint32_t max_pd_pages = (max_pd_bytes + PGSIZE - 1) / PGSIZE;

  pde_t* pg_dir_top = (pde_t*)p_buff;
  memset(page_dir_top, 0, max_pd_bytes);

  //mark these frames as occupied in the virtual/physical bitmaps
  uint32_t max_frames_in_bytes = max_pd_bytes / 8;
  memset(p_bmap, 0xFF, max_frames_in_bytes);

  uint32_t remainder_frames = max_pd_bytes % 8;
  for(int i = 0; i < remainder_frames; i++) {
    set_bit(p_bmap, (max_frames_in_bytes * 8) + i);
  }
  
}

// -----------------------------------------------------------------------------
// TLB
// -----------------------------------------------------------------------------

/*
 * TLB_add()
 * ---------
 * Adds a new virtual-to-physical translation to the TLB.
 * Ensure thread safety when updating shared TLB data.
 *
 * Return:
 *   0  -> Success (translation successfully added)
 *  -1  -> Failure (e.g., TLB full or invalid input)
 */
int TLB_add(void *va, void *pa)
{
    // TODO: Implement TLB insertion logic.
    return -1; // Currently returns failure placeholder.
}

/*
 * TLB_check()
 * -----------
 * Looks up a virtual address in the TLB.
 *
 * Return:
 *   Pointer to the corresponding page table entry (PTE) if found.
 *   NULL if the translation is not found (TLB miss).
 */
pte_t *TLB_check(void *va)
{
    // TODO: Implement TLB lookup.
    return NULL; // Currently returns TLB miss.
}

/*
 * print_TLB_missrate()
 * --------------------
 * Calculates and prints the TLB miss rate.
 *
 * Return value: None.
 */
void print_TLB_missrate(void)
{
    double miss_rate = 0.0;
    // TODO: Calculate miss rate as (tlb_misses / tlb_lookups).
    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}

// -----------------------------------------------------------------------------
// Page Table
// -----------------------------------------------------------------------------

/*
 * translate()
 * -----------
 * Translates a virtual address to a physical address.
 * Perform a TLB lookup first; if not found, walk the page directory
 * and page tables using a two-level lookup.
 *
 * Return:
 *   Pointer to the PTE structure if translation succeeds.
 *   NULL if translation fails (e.g., page not mapped).
 */
pte_t *translate(pde_t *pgdir, void *va)
{
    // TODO: Extract the 32-bit virtual address and compute indices
    // for the page directory, page table, and offset.
    // Return the corresponding PTE if found.
    return NULL; // Translation unsuccessful placeholder.
}

/*
 * map_page()
 * -----------
 * Establishes a mapping between a virtual and a physical page.
 * Creates intermediate page tables if necessary.
 *
 * Return:
 *   0  -> Success (mapping created)
 *  -1  -> Failure (e.g., no space or invalid address)
 */
int map_page(pde_t *pgdir, void *va, void *pa)
{
    // TODO: Map virtual address to physical address in the page tables.
    return -1; // Failure placeholder.
}

// -----------------------------------------------------------------------------
// Allocation
// -----------------------------------------------------------------------------

/*
 * get_next_avail()
 * ----------------
 * Finds and returns the base virtual address of the next available
 * block of contiguous free pages.
 *
 * Return:
 *   Pointer to the base virtual address if available.
 *   NULL if there are no sufficient free pages.
 */
void *get_next_avail(int num_pages)
{
    // TODO: Implement virtual bitmap search for free pages.
    return NULL; // No available block placeholder.
}

/*
 * n_malloc()
 * -----------
 * Allocates a given number of bytes in virtual memory.
 * Initializes physical memory and page directories if not already done.
 *
 * Return:
 *   Pointer to the starting virtual address of allocated memory (success).
 *   NULL if allocation fails.
 */
void *n_malloc(unsigned int num_bytes)
{
    // TODO: Determine required pages, allocate them, and map them.
    return NULL; // Allocation failure placeholder.
}

/*
 * n_free()
 * ---------
 * Frees one or more pages of memory starting at the given virtual address.
 * Marks the corresponding virtual and physical pages as free.
 * Removes the translation from the TLB.
 *
 * Return value: None.
 */
void n_free(void *va, int size)
{
    // TODO: Clear page table entries, update bitmaps, and invalidate TLB.
    


}

// -----------------------------------------------------------------------------
// Data Movement
// -----------------------------------------------------------------------------

/*
 * put_data()
 * ----------
 * Copies data from a user buffer into simulated physical memory using
 * the virtual address. Handle page boundaries properly.
 *
 * Return:
 *   0  -> Success (data written successfully)
 *  -1  -> Failure (e.g., translation failure)
 */
int put_data(void *va, void *val, int size)
{
    // TODO: Walk virtual pages, translate to physical addresses,
    // and copy data into simulated memory.

    return -1; // Failure placeholder.
}

/*
 * get_data()
 * -----------
 * Copies data from simulated physical memory (accessed via virtual address)
 * into a user buffer.
 *
 * Return value: None.
 */
void get_data(void *va, void *val, int size)
{
    // TODO: Perform reverse operation of put_data().
    //
}

// -----------------------------------------------------------------------------
// Matrix Multiplication
// -----------------------------------------------------------------------------

/*
 * mat_mult()
 * ----------
 * Performs matrix multiplication of two matrices stored in virtual memory.
 * Each element is accessed and stored using get_data() and put_data().
 *
 * Return value: None.
 */
void mat_mult(void *mat1, void *mat2, int size, void *answer)
{
    int i, j, k;
    uint32_t a, b, c;

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            c = 0;
            for (k = 0; k < size; k++) {
                // TODO: Compute addresses for mat1[i][k] and mat2[k][j].
                // Retrieve values using get_data() and perform multiplication.
                get_data(NULL, &a, sizeof(int));  // placeholder
                get_data(NULL, &b, sizeof(int));  // placeholder
                c += (a * b);
            }
            // TODO: Store the result in answer[i][j] using put_data().
            put_data(NULL, (void *)&c, sizeof(int)); // placeholder
        }
    }
}


// -----------------------------------------------------------------------------
// Helper Functions 
// -----------------------------------------------------------------------------

// bitmap getters/setters
void set_bit(char* bmap, int idx) { 
  uint32_t target_byte = idx / 8;
  uint8_t target_bit = idx % 8;
  bmap[target_byte] |= (1 << (target_bit));
  
}

void clear_bit(char* bmap, int idx) {
  uint32_t target_byte = bmap[idx / 8];
  uint8_t target_bit  = idx % 8;
  bmap[target_byte] &= ~(1 << (target_bit));
}

int get_bit(char* bmap, int idx) {
  uint32_t target_byte = idx / 8;
  uint8_t target_bit  = idx % 8;
  return bmap[target_byte] & (1 << (target_bit);
}

