#ifndef SOLIX_MM_H
#define SOLIX_MM_H

#include "types.h"

// Memory management constants
#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024
#define KERNEL_HEAP_SIZE (16 * 1024 * 1024)  // 16MB kernel heap

// Page directory and table entries
typedef struct page_entry {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t pwt        : 1;
    uint32_t pcd        : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t pat        : 1;
    uint32_t global     : 1;
    uint32_t avail      : 3;
    uint32_t frame      : 20;
} __attribute__((packed)) page_entry_t;

typedef struct page_table {
    page_entry_t pages[PAGE_ENTRIES];
} page_table_t;

typedef struct page_directory {
    page_table_t* tables[PAGE_ENTRIES];
    page_entry_t entries[PAGE_ENTRIES];
    uint32_t physical_addr;
} page_directory_t;

// Memory management functions
void mm_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* kmalloc_aligned(size_t size, size_t alignment);
void kfree_aligned(void* ptr);

// Paging functions
void paging_init(uint32_t mem_size);
void* alloc_frame(void);
void free_frame(void* frame);
void map_page(page_directory_t* dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
void unmap_page(page_directory_t* dir, uint32_t virt_addr);

// Heap management
void heap_init(void);
void* heap_alloc(size_t size);
void heap_free(void* ptr);

// Memory diagnostics and statistics
void print_memory_stats(void);
bool verify_heap_integrity(void);

#endif
