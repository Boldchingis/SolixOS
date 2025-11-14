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

// Current page directory
static page_directory_t* current_directory;

// Simple heap block header
typedef struct heap_block {
    uint32_t size;
    uint32_t used;
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

static heap_block_t* heap_head = NULL;

// Initialize memory management
void mm_init(void) {
    // Initialize heap
    heap_init();
    
    // Initialize paging (assuming 128MB RAM for now)
    paging_init(128 * 1024 * 1024);
}

// Initialize heap
void heap_init(void) {
    // Place heap after kernel image
    extern uint32_t __end;
    kernel_heap_start = (uint32_t)&__end;
    kernel_heap_end = kernel_heap_start + KERNEL_HEAP_SIZE;
    kernel_heap_current = kernel_heap_start;
    
    // Initialize first heap block
    heap_head = (heap_block_t*)kernel_heap_start;
    heap_head->size = KERNEL_HEAP_SIZE - sizeof(heap_block_t);
    heap_head->used = 0;
    heap_head->next = NULL;
    heap_head->prev = NULL;
}

// Simple kernel memory allocator
void* kmalloc(size_t size) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;
    
    heap_block_t* block = heap_head;
    
    // Find suitable block
    while (block) {
        if (!block->used && block->size >= size) {
            // Split block if necessary
            if (block->size > size + sizeof(heap_block_t) + 4) {
                heap_block_t* new_block = (heap_block_t*)((uint32_t)block + sizeof(heap_block_t) + size);
                new_block->size = block->size - size - sizeof(heap_block_t);
                new_block->used = 0;
                new_block->next = block->next;
                new_block->prev = block;
                
                if (block->next) {
                    block->next->prev = new_block;
                }
                
                block->size = size;
                block->next = new_block;
            }
            
            block->used = 1;
            return (void*)((uint32_t)block + sizeof(heap_block_t));
        }
        block = block->next;
    }
    
    return NULL; // Out of memory
}

// Free kernel memory
void kfree(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((uint32_t)ptr - sizeof(heap_block_t));
    block->used = 0;
    
    // Merge with next block if free
    if (block->next && !block->next->used) {
        block->size += block->next->size + sizeof(heap_block_t);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // Merge with previous block if free
    if (block->prev && !block->prev->used) {
        block->prev->size += block->size + sizeof(heap_block_t);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

// Initialize paging
void paging_init(uint32_t mem_size) {
    total_frames = mem_size / PAGE_SIZE;
    used_frames = 0;
    
    // Allocate frame bitmap
    frame_bitmap = (uint32_t*)kmalloc((total_frames + 31) / 32 * 4);
    
    // Clear bitmap
    for (uint32_t i = 0; i < (total_frames + 31) / 32; i++) {
        frame_bitmap[i] = 0;
    }
    
    // Create kernel page directory
    current_directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    
    // Clear page directory
    for (int i = 0; i < PAGE_ENTRIES; i++) {
        current_directory->tables[i] = NULL;
        current_directory->entries[i].present = 0;
    }
    
    // Identity map first 4MB
    for (uint32_t i = 0; i < 1024; i++) {
        map_page(current_directory, i * PAGE_SIZE, i * PAGE_SIZE, 0x03); // Present + RW
    }
    
    // Enable paging
    uint32_t cr0;
    __asm__ volatile("mov %%cr3, %0" : "=r" (current_directory->physical_addr));
    __asm__ volatile("mov %0, %%cr3" : : "r" (current_directory->physical_addr));
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 0x80000000; // Enable paging bit
    __asm__ volatile("mov %0, %%cr0" : : "r" (cr0));
}

// Allocate a physical frame
void* alloc_frame(void) {
    for (uint32_t i = 0; i < total_frames; i++) {
        uint32_t frame_index = i / 32;
        uint32_t bit_index = i % 32;
        
        if (!(frame_bitmap[frame_index] & (1 << bit_index))) {
            frame_bitmap[frame_index] |= (1 << bit_index);
            used_frames++;
            return (void*)(i * PAGE_SIZE);
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

// Aligned memory allocation
void* kmalloc_aligned(size_t size, size_t alignment) {
    uint32_t addr = (uint32_t)kmalloc(size + alignment - 1);
    return (void*)((addr + alignment - 1) & ~(alignment - 1));
}

// Free aligned memory
void kfree_aligned(void* ptr) {
    kfree(ptr);
}
