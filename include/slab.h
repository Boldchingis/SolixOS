#ifndef SOLIX_SLAB_H
#define SOLIX_SLAB_H

#include "types.h"
#include "mm.h"

/**
 * Linux-Inspired SLAB Allocator for SolixOS
 * Implements efficient kernel memory management with object caching
 * Based on Linux SLAB allocator design principles
 */

// SLAB allocator constants
#define SLAB_HWCACHE_ALIGN    1
#define SLAB_CACHE_DMA        2
#define SLAB_CACHE_DESTROY   4
#define SLAB_PANIC           8

// Object alignment
#define CACHE_LINE_SIZE       64
#define BYTES_PER_WORD        sizeof(void*)
#define SLAB_LIMIT            (((PAGE_SIZE) - sizeof(struct slab)) / (sizeof(void*)))

// SLAB colors for cache line alignment
#define SLAB_NUM_COLORS       8

/**
 * SLAB cache structure
 */
typedef struct kmem_cache {
    // Cache configuration
    const char *name;              // Cache name for debugging
    size_t object_size;            // Size of each object
    size_t align;                  // Object alignment requirement
    
    // Cache management
    struct list_head slabs_full;   // Full slabs
    struct list_head slabs_partial; // Partially full slabs
    struct list_head slabs_free;    // Empty slabs
    
    // Object management
    void (*ctor)(void *obj);       // Object constructor
    void (*dtor)(void *obj);       // Object destructor
    
    // Cache statistics
    unsigned int num;              // Number of objects per slab
    unsigned int batchcount;       // Objects to allocate/free in batch
    unsigned int limit;            // Upper limit on free objects
    
    // Color management for cache line alignment
    unsigned int colour_next;      // Next color to use
    unsigned int colour_off;       // Color offset
    unsigned int colour;           // Number of colors
    
    // Cache flags
    unsigned int flags;
    
    // Memory management
    size_t gfporder;               // Order of pages to allocate
    size_t gfpflags;               // GFP flags for allocation
    
    // Cache statistics
    struct {
        unsigned int allocated;    // Total allocated objects
        unsigned int freed;        // Total freed objects
        unsigned int errors;       // Allocation errors
        unsigned int max_active;   // Maximum active objects
        unsigned int active;       // Currently active objects
    } stats;
    
} kmem_cache_t;

/**
 * Individual slab structure
 */
struct slab {
    struct list_head list;         // List linkage
    kmem_cache_t *slab_cache;      // Parent cache
    void *s_mem;                   // Start of memory
    unsigned int inuse;            // Number of objects in use
    unsigned int colouroff;        // Color offset
    
    // Object tracking
    void **freelist;               // Free object list
    unsigned int objects;          // Total objects in this slab
    
    // Debug information
    unsigned int magic;            // Magic number for corruption detection
    void *end;                     // End of slab memory
};

/**
 * Generic cache definitions
 */
extern kmem_cache_t *kmem_cache_create(const char *name, size_t size, 
                                     size_t align, unsigned long flags,
                                     void (*ctor)(void *), void (*dtor)(void *));
extern void kmem_cache_destroy(kmem_cache_t *cachep);

/**
 * Object allocation and deallocation
 */
extern void *kmem_cache_alloc(kmem_cache_t *cachep, unsigned long flags);
extern void kmem_cache_free(kmem_cache_t *cachep, void *objp);

/**
 * Bulk operations for performance
 */
extern int kmem_cache_alloc_bulk(kmem_cache_t *cachep, unsigned long flags,
                                size_t size, void **p);
extern void kmem_cache_free_bulk(kmem_cache_t *cachep, size_t size, void **p);

/**
 * Cache information and debugging
 */
extern unsigned int kmem_cache_size(kmem_cache_t *cachep);
extern const char *kmem_cache_name(kmem_cache_t *cachep);
extern void kmem_cache_info(kmem_cache_t *cachep);
extern void kmem_cache_debug(kmem_cache_t *cachep);

/**
 * Common kernel caches
 */
extern kmem_cache_t *kmalloc_caches[12];
extern kmem_cache_t *vm_area_cache;
extern kmem_cache_t *files_cache;
extern kmem_cache_t *fs_cache;
extern kmem_cache_t *sighand_cache;
extern kmem_cache_t *signal_cache;
extern kmem_cache_t *mm_struct_cache;
extern kmem_cache_t *names_cache;
extern kmem_cache_t *dquot_cache;
extern kmem_cache_t *dentry_cache;
extern kmem_cache_t *inode_cache;
extern kmem_cache_t *file_lock_cache;
extern kmem_cache_t *buffer_head_cache;
extern kmem_cache_t *uid_cache;
extern kmem_cache_t *pid_cache;

/**
 * kmalloc/kfree interface using SLAB
 */
static inline void *kmalloc_slab(size_t size, unsigned long flags) {
    if (size == 0) return NULL;
    
    // Find appropriate cache based on size
    int i = 0;
    while (i < 11 && kmalloc_caches[i] && size > kmem_cache_size(kmalloc_caches[i])) {
        i++;
    }
    
    if (i >= 11 || !kmalloc_caches[i]) {
        return NULL; // Size too large
    }
    
    return kmem_cache_alloc(kmalloc_caches[i], flags);
}

static inline void kfree_slab(void *objp) {
    if (!objp) return;
    
    // Find which cache this object belongs to (simplified)
    // In a real implementation, we'd store cache metadata
    struct slab *slabp = (struct slab *)((unsigned long)objp & ~(PAGE_SIZE - 1));
    kmem_cache_free(slabp->slab_cache, objp);
}

/**
 * SLAB allocator initialization
 */
extern void slab_init(void);
extern void kmem_cache_init(void);

/**
 * Memory reclaim and management
 */
extern void kmem_cache_reap(void);
extern void kmem_cache_shrink(kmem_cache_t *cachep);
extern size_t kmem_cache_estimate(unsigned long gfporder, size_t size,
                                  size_t align, unsigned long flags);

/**
 * Debugging and validation
 */
extern void kmem_cache_validate(kmem_cache_t *cachep);
extern void kmem_cache_verify(void);
extern void slab_debug_info(void);

/**
 * Configuration options
 */
#define SLAB_DEBUG          1
#define SLAB_STATS           2
#define SLAB_RED_ZONE        4
#define SLAB_POISON          8
#define SLAB_HWCACHE_ALIGN   16

/**
 * GFP flags (Linux-inspired)
 */
#define GFP_NOWAIT          0x01
#define GFP_WAIT            0x02
#define GFP_IO              0x04
#define GFP_FS              0x08
#define GFP_HIGHUSER        0x10
#define GFP_KERNEL          (GFP_WAIT | GFP_IO | GFP_FS)
#define GFP_ATOMIC          (GFP_NOWAIT | GFP_IO)
#define GFP_USER            (GFP_WAIT | GFP_IO | GFP_FS | GFP_HIGHUSER)

/**
 * Helper macros
 */
#define SLAB_BATCH_COUNT    16
#define SLAB_MAX_ORDER      5
#define SLAB_PAGE_SHIFT     (PAGE_SHIFT)

#define SLAB_BYTES         (1 << SLAB_PAGE_SHIFT)
#define SLAB_OBJ_PER_PAGE  (SLAB_BYTES / sizeof(void*))

#define CACHE(x)           (kmalloc_caches[x])
#define KMALLOC_MAX_SIZE   (PAGE_SIZE * 2)
#define KMALLOC_SHIFT_HIGH PAGE_SHIFT

/**
 * Size classes for kmalloc
 */
#define KMALLOC_SHIFT_LOW  3
#define KMALLOC_SHIFT_HIGH PAGE_SHIFT

/**
 * Alignment helpers
 */
#define ALIGN(x, a)        (((x) + (a) - 1) & ~((a) - 1))
#define SLAB_ALIGN(x, a)   ALIGN(x, (a))

/**
 * Memory barriers for SMP safety (preparation)
 */
#define smp_mb()           __asm__ volatile("mfence" ::: "memory")
#define smp_rmb()          __asm__ volatile("lfence" ::: "memory")
#define smp_wmb()          __asm__ volatile("sfence" ::: "memory")

/**
 * Locking primitives for SLAB
 */
typedef struct {
    volatile unsigned int lock;
} spinlock_t;

#define SPIN_LOCK_UNLOCKED  { 0 }

static inline void spin_lock(spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        // Spin until lock is acquired
    }
}

static inline void spin_unlock(spinlock_t *lock) {
    __sync_lock_release(&lock->lock);
}

static inline int spin_trylock(spinlock_t *lock) {
    return !__sync_lock_test_and_set(&lock->lock, 1);
}

#endif
