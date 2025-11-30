
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

static pde_t* pgdir = NULL;
static pthread_mutex_t lock;

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
  memset(p_bmap, 0, p_bmap_bytes);

  uint32_t v_bmap_bytes = ((MAX_MEMSIZE / PGSIZE) + 7) / 8;
  v_bmap = malloc(v_bmap_bytes);
  memset(v_bmap, 0, v_bmap_bytes);
  
  // the top frame(s) are reserved for the page directory (pgdir)
  uint32_t max_pd_entries = 1 << PDX_BITS;
  uint32_t max_pd_bytes = max_pd_entries * sizeof(pde_t);
  uint32_t max_pd_pages = (max_pd_bytes + PGSIZE - 1) / PGSIZE;

  pgdir = (pde_t*)p_buff;
  memset(pgdir, 0, max_pd_bytes);

  //mark these frames as occupied in the virtual/physical bitmaps
  uint32_t max_frames_in_bytes = max_pd_pages / 8;
  memset(p_bmap, 0xFF, max_frames_in_bytes);

  uint32_t remainder_frames = max_pd_pages % 8;
  for(int i = 0; i < remainder_frames; i++) {
    set_bit(p_bmap, (max_frames_in_bytes * 8) + i);
  }

  pthread_mutex_init(&lock, NULL);
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
pte_t* translate(pde_t* pgdir, void* va)
{
    // extract the 32-bit virtual address and compute indices
    // for the page directory, page table, and offset.
    // return the corresponding PTE (i.e. p_frame) if found
 
    vaddr32_t v_addr = VA2U(va);

    uint32_t pgdir_idx = PDX(v_addr);
    pde_t pgdir_entry = pgdir[pgdir_idx];
    if(pgdir_entry == 0) return NULL;

    uint32_t pgtbl_idx = PTX(v_addr);
    pte_t* pgtbl = (pte_t*)(pgdir_entry & ~OFFMASK);
    pte_t* pgtbl_entry_ptr = &(pgtbl[pgtbl_idx]);
    if(*pgtbl_entry_ptr == 0) return NULL;

    return pgtbl_entry_ptr;
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
      if(pgtbl_frame == NULL) return -1;
  
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
    if(num_bytes == 0) return NULL;
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
int put_data(void *va, void *val, int size) {
  return copy_data(va, val, size, 1);
}

/*
 * get_data()
 * -----------
 * Copies data from simulated physical memory (accessed via virtual address)
 * into a user buffer.
 *
 * Return value: None.
 */
void get_data(void *va, void *val, int size) {
  copy_data(va, val, size, 0);
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
  uint32_t target_byte = idx / 8;
  uint8_t target_bit  = idx % 8;
  bmap[target_byte] &= ~(1 << (target_bit));
}

int get_bit(char* bmap, int idx) {
  uint32_t target_byte = idx / 8;
  uint8_t target_bit  = idx % 8;
  return bmap[target_byte] & (1 << (target_bit));
}

void* alloc_frame() {
  for(uint32_t i = 0; i < MAX_NUM_FRAMES; i++) {
    if(get_bit(p_bmap, i) == 0) {
      set_bit(p_bmap, i);
      return (void*)(p_buff + (i * PGSIZE));
    }
  }

  return NULL;        // bitmap is full (i.e. out of memory) 
}

static int copy_data(void* va, void* val, int size, int dir) {
  if(dir != 0 && dir != 1) return -1;

  if(va == NULL || val == NULL || size <= 0)
    return -1;

  vaddr32_t va_base = VA2U(va);
  int num_bytes_written = 0;

  while(num_bytes_written < size) {
    pte_t* pte = translate(pgdir, U2VA(va_base));
    if(pte == NULL) return -1;

    uint32_t offset = OFF(va_base);
    uint32_t rem_frame_bytes = PGSIZE - offset;
    
    uint32_t chunk_size; 
    if((size - num_bytes_written) <= rem_frame_bytes) {
      chunk_size = size - num_bytes_written; 
    } else {
      chunk_size = rem_frame_bytes;
    }

    paddr32_t pa = (*pte & ~OFFMASK) + offset;
    void* pa_ptr = (void*)(uintptr_t)pa;
    void* ext_ptr = val + num_bytes_written;

    if(dir == 1) {
      memcpy(pa_ptr, ext_ptr, chunk_size);
    } else {
      memcpy(ext_ptr, pa_ptr, chunk_size);
    }

    va_base += chunk_size;
    num_bytes_written += chunk_size;
  }
  return 0; 


}

