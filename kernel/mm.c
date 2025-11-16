#include "mm.h"
#include "kernel.h"

// Memory management state
static uint32_t kernel_heap_start;
static uint32_t kernel_heap_end;
static uint32_t kernel_heap_current;

// Frame allocation bitmap
static uint32_t* frame_bitmap;
static uint32_t total_frames;
static uint32_t used_frames;
static uint32_t last_alloc_frame;  // Optimization: start search from last allocation

// Current page directory
static page_directory_t* current_directory;

// Memory statistics
static struct {
    uint32_t total_allocations;
    uint32_t total_frees;
    uint32_t peak_usage;
    uint32_t current_usage;
    uint32_t fragmentation_count;
} mem_stats = {0};

// Heap block header with magic numbers for corruption detection
#define HEAP_BLOCK_MAGIC 0xDEADBEEF
#define HEAP_BLOCK_FREE_MAGIC 0xFEEDFACE

typedef struct heap_block {
    uint32_t magic;           // Corruption detection
    uint32_t size;
    uint32_t used;
    struct heap_block* next;
    struct heap_block* prev;
    uint32_t checksum;        // Simple integrity check
} heap_block_t;

static heap_block_t* heap_head = NULL;
static heap_block_t* heap_tail = NULL;

// Initialize memory management
void mm_init(void) {
    // Initialize heap
    heap_init();
    
    // Initialize paging (assuming 128MB RAM for now)
    paging_init(128 * 1024 * 1024);
}

// Calculate simple checksum for heap block integrity
static uint32_t calculate_block_checksum(heap_block_t* block) {
    uint32_t checksum = 0;
    uint8_t* data = (uint8_t*)block;
    for (size_t i = 0; i < sizeof(heap_block_t) - sizeof(uint32_t); i++) {
        checksum += data[i];
    }
    return checksum;
}

// Verify heap block integrity
static bool verify_block_integrity(heap_block_t* block) {
    if (!block) return false;
    
    uint32_t expected_magic = block->used ? HEAP_BLOCK_MAGIC : HEAP_BLOCK_FREE_MAGIC;
    if (block->magic != expected_magic) {
        return false;
    }
    
    uint32_t expected_checksum = calculate_block_checksum(block);
    return block->checksum == expected_checksum;
}

// Update block checksum after modification
static void update_block_checksum(heap_block_t* block) {
    if (block) {
        block->checksum = calculate_block_checksum(block);
    }
}

// Initialize heap with enhanced error checking
void heap_init(void) {
    // Place heap after kernel image
    extern uint32_t __end;
    kernel_heap_start = (uint32_t)&__end;
    kernel_heap_end = kernel_heap_start + KERNEL_HEAP_SIZE;
    kernel_heap_current = kernel_heap_start;

    // Validate heap size
    if (kernel_heap_end <= kernel_heap_start) {
        panic("Invalid heap size configuration");
    }

    // Initialize first heap block with enhanced metadata
    heap_head = (heap_block_t*)kernel_heap_start;
    heap_tail = heap_head;
    
    heap_head->magic = HEAP_BLOCK_FREE_MAGIC;
    heap_head->size = KERNEL_HEAP_SIZE - sizeof(heap_block_t);
    heap_head->used = 0;
    heap_head->next = NULL;
    heap_head->prev = NULL;
    update_block_checksum(heap_head);
    
    // Initialize statistics
    mem_stats.total_allocations = 0;
    mem_stats.total_frees = 0;
    mem_stats.current_usage = 0;
    mem_stats.peak_usage = 0;
    mem_stats.fragmentation_count = 0;
}

// Enhanced kernel memory allocator with first-fit strategy and integrity checks
void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Align size to 4 bytes and add minimum allocation size
    size = (size + 3) & ~3;
    if (size < 16) size = 16;  // Minimum allocation size

    heap_block_t* block = heap_head;
    heap_block_t* best_fit = NULL;
    size_t best_fit_size = SIZE_MAX;

    // Find best fitting block (reduces fragmentation)
    while (block) {
        if (!block->used && block->size >= size) {
            if (block->size < best_fit_size) {
                best_fit = block;
                best_fit_size = block->size;
                
                // Perfect fit found
                if (best_fit_size == size) break;
            }
        }
        block = block->next;
    }

    if (!best_fit) {
        mem_stats.fragmentation_count++;
        return NULL; // Out of memory
    }

    block = best_fit;
    
    // Verify block integrity before use
    if (!verify_block_integrity(block)) {
        panic("Heap corruption detected in kmalloc");
    }

    // Split block if necessary
    if (block->size > size + sizeof(heap_block_t) + 16) {
        heap_block_t* new_block = (heap_block_t*)((uint32_t)block + sizeof(heap_block_t) + size);
        
        new_block->magic = HEAP_BLOCK_FREE_MAGIC;
        new_block->size = block->size - size - sizeof(heap_block_t);
        new_block->used = 0;
        new_block->next = block->next;
        new_block->prev = block;
        update_block_checksum(new_block);

        if (block->next) {
            block->next->prev = new_block;
        } else {
            heap_tail = new_block;
        }

        block->size = size;
        block->next = new_block;
        update_block_checksum(block);
    }

    // Mark block as used
    block->used = 1;
    block->magic = HEAP_BLOCK_MAGIC;
    update_block_checksum(block);

    // Update statistics
    mem_stats.total_allocations++;
    mem_stats.current_usage += size;
    if (mem_stats.current_usage > mem_stats.peak_usage) {
        mem_stats.peak_usage = mem_stats.current_usage;
    }

    return (void*)((uint32_t)block + sizeof(heap_block_t));
}

// Enhanced free with corruption detection and immediate coalescing
void kfree(void* ptr) {
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)((uint32_t)ptr - sizeof(heap_block_t));
    
    // Verify block integrity
    if (!verify_block_integrity(block)) {
        panic("Heap corruption detected in kfree");
    }
    
    if (!block->used) {
        panic("Double free detected");
    }

    size_t freed_size = block->size;
    
    // Mark block as free
    block->used = 0;
    block->magic = HEAP_BLOCK_FREE_MAGIC;
    update_block_checksum(block);

    // Immediate coalescing with next block if free
    if (block->next && !block->next->used && verify_block_integrity(block->next)) {
        block->size += block->next->size + sizeof(heap_block_t);
        block->next = block->next->next;
        
        if (block->next) {
            block->next->prev = block;
        } else {
            heap_tail = block;
        }
        update_block_checksum(block);
    }

    // Immediate coalescing with previous block if free
    if (block->prev && !block->prev->used && verify_block_integrity(block->prev)) {
        block->prev->size += block->size + sizeof(heap_block_t);
        block->prev->next = block->next;
        
        if (block->next) {
            block->next->prev = block->prev;
        } else {
            heap_tail = block->prev;
        }
        update_block_checksum(block->prev);
    }

    // Update statistics
    mem_stats.total_frees++;
    mem_stats.current_usage -= freed_size;
}

// Enhanced paging initialization with better error handling
void paging_init(uint32_t mem_size) {
    if (mem_size < 4 * 1024 * 1024) {
        panic("Insufficient memory for paging (minimum 4MB required)");
    }
    
    total_frames = mem_size / PAGE_SIZE;
    used_frames = 0;
    last_alloc_frame = 0;

    // Calculate bitmap size and allocate
    size_t bitmap_size = (total_frames + 31) / 32 * 4;
    frame_bitmap = (uint32_t*)kmalloc(bitmap_size);
    
    if (!frame_bitmap) {
        panic("Failed to allocate frame bitmap");
    }

    // Clear bitmap
    for (uint32_t i = 0; i < bitmap_size / 4; i++) {
        frame_bitmap[i] = 0;
    }

    // Create kernel page directory
    current_directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    
    if (!current_directory) {
        panic("Failed to allocate page directory");
    }
    
    // Store physical address for CR3
    current_directory->physical_addr = (uint32_t)current_directory;

    // Clear page directory
    for (int i = 0; i < PAGE_ENTRIES; i++) {
        current_directory->tables[i] = NULL;
        current_directory->entries[i].present = 0;
        current_directory->entries[i].rw = 0;
        current_directory->entries[i].user = 0;
    }

    // Identity map first 4MB with proper permissions
    for (uint32_t i = 0; i < 1024; i++) {
        map_page(current_directory, i * PAGE_SIZE, i * PAGE_SIZE, 0x03); // Present + RW
    }

    // Enable paging with error checking
    uint32_t cr0;
    __asm__ volatile("mov %%cr3, %0" : "=r" (current_directory->physical_addr));
    __asm__ volatile("mov %0, %%cr3" : : "r" (current_directory->physical_addr));
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
    
    if (cr0 & 0x80000000) {
        panic("Paging already enabled");
    }
    
    cr0 |= 0x80000000; // Enable paging bit
    __asm__ volatile("mov %0, %%cr0" : : "r" (cr0));
}

// Optimized frame allocation with next-fit strategy
void* alloc_frame(void) {
    // Start search from last allocation position for better locality
    uint32_t start_frame = last_alloc_frame;
    
    for (uint32_t i = 0; i < total_frames; i++) {
        uint32_t frame_index = (start_frame + i) % total_frames;
        uint32_t bitmap_index = frame_index / 32;
        uint32_t bit_index = frame_index % 32;

        if (!(frame_bitmap[bitmap_index] & (1 << bit_index))) {
            frame_bitmap[bitmap_index] |= (1 << bit_index);
            used_frames++;
            last_alloc_frame = frame_index;
            return (void*)(frame_index * PAGE_SIZE);
        }
    }

    return NULL; // Out of frames
}

// Free a physical frame
void free_frame(void* frame) {
    uint32_t frame_addr = (uint32_t)frame;
    uint32_t frame_index = frame_addr / PAGE_SIZE;
    uint32_t bitmap_index = frame_index / 32;
    uint32_t bit_index = frame_index % 32;
    
    frame_bitmap[bitmap_index] &= ~(1 << bit_index);
    used_frames--;
}

// Map a virtual page to a physical frame
void map_page(page_directory_t* dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t page_index = virt_addr / PAGE_SIZE;
    uint32_t table_index = page_index / PAGE_ENTRIES;
    uint32_t entry_index = page_index % PAGE_ENTRIES;
    
    // Create page table if it doesn't exist
    if (!dir->tables[table_index]) {
        dir->tables[table_index] = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        
        // Clear page table
        for (int i = 0; i < PAGE_ENTRIES; i++) {
            dir->tables[table_index]->pages[i].present = 0;
        }
        
        // Update directory entry
        dir->entries[table_index].frame = (uint32_t)dir->tables[table_index] >> 12;
        dir->entries[table_index].present = 1;
        dir->entries[table_index].rw = 1;
        dir->entries[table_index].user = 0;
    }
    
    // Map the page
    dir->tables[table_index]->pages[entry_index].frame = phys_addr >> 12;
    dir->tables[table_index]->pages[entry_index].present = (flags & 0x01) ? 1 : 0;
    dir->tables[table_index]->pages[entry_index].rw = (flags & 0x02) ? 1 : 0;
    dir->tables[table_index]->pages[entry_index].user = (flags & 0x04) ? 1 : 0;
    
    // Invalidate TLB
    __asm__ volatile("invlpg (%0)" : : "r" (virt_addr));
}

// Unmap a virtual page
void unmap_page(page_directory_t* dir, uint32_t virt_addr) {
    uint32_t page_index = virt_addr / PAGE_SIZE;
    uint32_t table_index = page_index / PAGE_ENTRIES;
    uint32_t entry_index = page_index % PAGE_ENTRIES;
    
    if (dir->tables[table_index]) {
        dir->tables[table_index]->pages[entry_index].present = 0;
        
        // Invalidate TLB
        __asm__ volatile("invlpg (%0)" : : "r" (virt_addr));
    }
}

// Enhanced aligned memory allocation with overflow protection
void* kmalloc_aligned(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0) return NULL;
    
    // Validate alignment is power of 2
    if (alignment & (alignment - 1)) {
        return NULL; // Alignment must be power of 2
    }
    
    // Calculate total needed size with header and alignment
    size_t total_size = size + alignment - 1 + sizeof(heap_block_t*) + sizeof(uint32_t);
    void* raw_ptr = kmalloc(total_size);
    
    if (!raw_ptr) return NULL;
    
    // Calculate aligned address
    uint32_t aligned_addr = ((uint32_t)raw_ptr + sizeof(heap_block_t*) + sizeof(uint32_t) + alignment - 1) & ~(alignment - 1);
    
    // Store original pointer before aligned address for proper freeing
    heap_block_t** original_ptr = (heap_block_t**)(aligned_addr - sizeof(heap_block_t*));
    *original_ptr = (heap_block_t*)((uint32_t)raw_ptr - sizeof(heap_block_t));
    
    // Store alignment info
    uint32_t* alignment_info = (uint32_t*)(aligned_addr - sizeof(heap_block_t*) - sizeof(uint32_t));
    *alignment_info = alignment;
    
    return (void*)aligned_addr;
}

// Enhanced aligned memory free
void kfree_aligned(void* ptr) {
    if (!ptr) return;
    
    // Retrieve original pointer and alignment info
    uint32_t aligned_addr = (uint32_t)ptr;
    heap_block_t** original_ptr = (heap_block_t**)(aligned_addr - sizeof(heap_block_t*));
    
    // Verify the alignment info matches
    uint32_t* alignment_info = (uint32_t*)(aligned_addr - sizeof(heap_block_t*) - sizeof(uint32_t));
    uint32_t alignment = *alignment_info;
    
    // Verify this is actually an aligned allocation
    if (aligned_addr % alignment != 0) {
        panic("Invalid aligned free - address not properly aligned");
    }
    
    // Free the original allocation
    kfree((void*)((uint32_t)*original_ptr + sizeof(heap_block_t)));
}

// Memory statistics and diagnostics
void print_memory_stats(void) {
    screen_print("\n=== Memory Statistics ===\n");
    screen_print("Total allocations: ");
    screen_print_dec(mem_stats.total_allocations);
    screen_print("\nTotal frees: ");
    screen_print_dec(mem_stats.total_frees);
    screen_print("\nCurrent usage: ");
    screen_print_dec(mem_stats.current_usage);
    screen_print(" bytes\nPeak usage: ");
    screen_print_dec(mem_stats.peak_usage);
    screen_print(" bytes\nFragmentation events: ");
    screen_print_dec(mem_stats.fragmentation_count);
    screen_print("\nFrames used: ");
    screen_print_dec(used_frames);
    screen_print("/");
    screen_print_dec(total_frames);
    screen_print("\n");
}

// Heap integrity check
bool verify_heap_integrity(void) {
    heap_block_t* block = heap_head;
    uint32_t block_count = 0;
    
    while (block) {
        if (!verify_block_integrity(block)) {
            return false;
        }
        
        // Check for circular references
        if (block_count > total_frames) {
            return false;
        }
        
        block_count++;
        block = block->next;
    }
    
    return true;
}
