#include "slab.h"
#include "kernel.h"
#include "mm.h"
#include "screen.h"

/**
 * Linux-Inspired SLAB Allocator Implementation
 * Efficient kernel memory management with object caching
 * Based on Linux SLAB allocator design principles
 */

// Global cache list for management
static LIST_HEAD(cache_chain);
static spinlock_t cache_chain_lock = SPIN_LOCK_UNLOCKED;

// Common kernel caches
kmem_cache_t *kmalloc_caches[12];
kmem_cache_t *vm_area_cache;
kmem_cache_t *files_cache;
kmem_cache_t *fs_cache;
kmem_cache_t *sighand_cache;
kmem_cache_t *signal_cache;
kmem_cache_t *mm_struct_cache;
kmem_cache_t *names_cache;
kmem_cache_t *dquot_cache;
kmem_cache_t *dentry_cache;
kmem_cache_t *inode_cache;
kmem_cache_t *file_lock_cache;
kmem_cache_t *buffer_head_cache;
kmem_cache_t *uid_cache;
kmem_cache_t *pid_cache;

// SLAB magic numbers for corruption detection
#define SLAB_RED_MAGIC1   0x5A5A5A5A
#define SLAB_RED_MAGIC2   0xA5A5A5A5
#define SLAB_POISON_BYTE   0x5A
#define SLAB_POISON_END    0xA5

/**
 * Calculate number of objects that fit in a slab
 */
static unsigned int calculate_objects(unsigned long order, size_t size, 
                                     size_t align, unsigned long flags) {
    unsigned int leftover;
    unsigned int objects;
    unsigned int nr_pages;
    
    nr_pages = 1 << order;
    
    // Account for slab header
    size_t slab_size = sizeof(struct slab);
    
    // Calculate objects per page
    objects = (PAGE_SIZE * nr_pages - slab_size) / size;
    
    // Account for color alignment
    if (flags & SLAB_HWCACHE_ALIGN) {
        leftover = (PAGE_SIZE * nr_pages - slab_size) % size;
        if (leftover >= sizeof(void*)) {
            objects++;
        }
    }
    
    return objects;
}

/**
 * Calculate slab colors for cache line alignment
 */
static unsigned int calculate_colours(kmem_cache_t *cachep, unsigned long *colour_off) {
    unsigned int colour = 0;
    
    if (cachep->flags & SLAB_HWCACHE_ALIGN) {
        unsigned int off = cachep->object_size % CACHE_LINE_SIZE;
        if (off) {
            colour = CACHE_LINE_SIZE / off;
            *colour_off = off;
        }
    }
    
    return colour;
}

/**
 * Allocate a new slab
 */
static struct slab* alloc_slab(kmem_cache_t *cachep, unsigned long flags) {
    struct slab *slabp;
    void *objp;
    unsigned int order = cachep->gfporder;
    
    // Allocate pages for the slab
    void *pages = kmalloc_aligned(PAGE_SIZE * (1 << order), PAGE_SIZE);
    if (!pages) {
        return NULL;
    }
    
    // Initialize slab structure at beginning
    slabp = (struct slab *)pages;
    slabp->slab_cache = cachep;
    slabp->inuse = 0;
    slabp->objects = cachep->num;
    slabp->magic = SLAB_RED_MAGIC1;
    
    // Calculate memory region for objects
    objp = (void *)((unsigned long)slabp + sizeof(struct slab) + cachep->colour_off);
    slabp->s_mem = objp;
    slabp->end = (void *)((unsigned long)pages + PAGE_SIZE * (1 << order));
    
    // Initialize freelist
    slabp->freelist = (void **)objp;
    
    // Set up free objects
    void **p = slabp->freelist;
    for (unsigned int i = 0; i < cachep->num - 1; i++) {
        *p = (void *)((unsigned long)p + cachep->object_size);
        p = (void **)*p;
    }
    *p = NULL; // End of list
    
    // Poison objects if debugging enabled
    if (cachep->flags & SLAB_POISON) {
        void *obj = objp;
        for (unsigned int i = 0; i < cachep->num; i++) {
            memset(obj, SLAB_POISON_BYTE, cachep->object_size);
            obj = (void *)((unsigned long)obj + cachep->object_size);
        }
    }
    
    // Call constructor for all objects if provided
    if (cachep->ctor) {
        void *obj = objp;
        for (unsigned int i = 0; i < cachep->num; i++) {
            cachep->ctor(obj);
            obj = (void *)((unsigned long)obj + cachep->object_size);
        }
    }
    
    cachep->stats.allocated += cachep->num;
    
    return slabp;
}

/**
 * Free a slab
 */
static void free_slab(kmem_cache_t *cachep, struct slab *slabp) {
    // Call destructor for all objects if provided
    if (cachep->dtor) {
        void *obj = slabp->s_mem;
        for (unsigned int i = 0; i < slabp->objects; i++) {
            cachep->dtor(obj);
            obj = (void *)((unsigned long)obj + cachep->object_size);
        }
    }
    
    // Free the pages
    kfree_aligned(slabp);
    
    cachep->stats.freed += slabp->objects;
}

/**
 * Grow cache by allocating new slabs
 */
static int cache_grow(kmem_cache_t *cachep, unsigned long flags) {
    struct slab *slabp;
    
    slabp = alloc_slab(cachep, flags);
    if (!slabp) {
        return -1;
    }
    
    // Add to appropriate list
    spin_lock(&cache_chain_lock);
    
    if (slabp->inuse == 0) {
        list_add(&slabp->list, &cachep->slabs_free);
    } else if (slabp->inuse == cachep->num) {
        list_add(&slabp->list, &cachep->slabs_full);
    } else {
        list_add(&slabp->list, &cachep->slabs_partial);
    }
    
    spin_unlock(&cache_chain_lock);
    
    return 0;
}

/**
 * Create a new cache
 */
kmem_cache_t* kmem_cache_create(const char *name, size_t size, 
                               size_t align, unsigned long flags,
                               void (*ctor)(void *), void (*dtor)(void *)) {
    kmem_cache_t *cachep;
    
    if (!name || size == 0 || size > KMALLOC_MAX_SIZE) {
        return NULL;
    }
    
    // Allocate cache structure
    cachep = (kmem_cache_t *)kmalloc(sizeof(kmem_cache_t));
    if (!cachep) {
        return NULL;
    }
    
    // Initialize cache structure
    memset(cachep, 0, sizeof(kmem_cache_t));
    
    cachep->name = name;
    cachep->object_size = size;
    cachep->align = align;
    cachep->flags = flags;
    cachep->ctor = ctor;
    cachep->dtor = dtor;
    
    // Initialize lists
    INIT_LIST_HEAD(&cachep->slabs_full);
    INIT_LIST_HEAD(&cachep->slabs_partial);
    INIT_LIST_HEAD(&cachep->slabs_free);
    
    // Calculate slab parameters
    cachep->gfporder = 0;
    cachep->num = calculate_objects(cachep->gfporder, size, align, flags);
    cachep->colour = calculate_colours(cachep, &cachep->colour_off);
    cachep->colour_next = 0;
    
    // Set batch count and limit
    cachep->batchcount = SLAB_BATCH_COUNT;
    cachep->limit = cachep->num * 2;
    
    // Validate cache parameters
    if (cachep->num == 0) {
        kfree(cachep);
        return NULL;
    }
    
    // Add to cache chain
    spin_lock(&cache_chain_lock);
    list_add(&cachep->list, &cache_chain);
    spin_unlock(&cache_chain_lock);
    
    debug_print(DEBUG_INFO, "Created cache '%s': objsize=%d, objs=%d, order=%d",
                name, size, cachep->num, cachep->gfporder);
    
    return cachep;
}

/**
 * Destroy a cache
 */
void kmem_cache_destroy(kmem_cache_t *cachep) {
    struct slab *slabp, *tmp;
    
    if (!cachep) return;
    
    // Free all slabs
    spin_lock(&cache_chain_lock);
    
    list_for_each_entry_safe(slabp, tmp, &cachep->slabs_full, list) {
        list_del(&slabp->list);
        free_slab(cachep, slabp);
    }
    
    list_for_each_entry_safe(slabp, tmp, &cachep->slabs_partial, list) {
        list_del(&slabp->list);
        free_slab(cachep, slabp);
    }
    
    list_for_each_entry_safe(slabp, tmp, &cachep->slabs_free, list) {
        list_del(&slabp->list);
        free_slab(cachep, slabp);
    }
    
    // Remove from cache chain
    list_del(&cachep->list);
    
    spin_unlock(&cache_chain_lock);
    
    debug_print(DEBUG_INFO, "Destroyed cache '%s'", cachep->name);
    
    kfree(cachep);
}

/**
 * Allocate object from cache
 */
void* kmem_cache_alloc(kmem_cache_t *cachep, unsigned long flags) {
    struct slab *slabp;
    void *objp;
    
    if (!cachep) return NULL;
    
    spin_lock(&cache_chain_lock);
    
    // Try to find object in partial slabs first
    if (!list_empty(&cachep->slabs_partial)) {
        slabp = list_first_entry(&cachep->slabs_partial, struct slab, list);
    } else if (!list_empty(&cachep->slabs_free)) {
        slabp = list_first_entry(&cachep->slabs_free, struct slab, list);
    } else {
        // Need to grow cache
        spin_unlock(&cache_chain_lock);
        
        if (cache_grow(cachep, flags) < 0) {
            cachep->stats.errors++;
            return NULL;
        }
        
        spin_lock(&cache_chain_lock);
        slabp = list_first_entry(&cachep->slabs_free, struct slab, list);
    }
    
    // Get object from freelist
    objp = slabp->freelist;
    if (!objp) {
        spin_unlock(&cache_chain_lock);
        cachep->stats.errors++;
        return NULL;
    }
    
    // Update freelist
    slabp->freelist = *((void **)objp);
    slabp->inuse++;
    
    // Move slab to appropriate list
    if (slabp->inuse == cachep->num) {
        list_del(&slabp->list);
        list_add(&slabp->list, &cachep->slabs_full);
    } else if (slabp->inuse == 1) {
        list_del(&slabp->list);
        list_add(&slabp->list, &cachep->slabs_partial);
    }
    
    spin_unlock(&cache_chain_lock);
    
    // Clear poison if debugging enabled
    if (cachep->flags & SLAB_POISON) {
        memset(objp, 0, cachep->object_size);
    }
    
    // Update statistics
    cachep->stats.active++;
    if (cachep->stats.active > cachep->stats.max_active) {
        cachep->stats.max_active = cachep->stats.active;
    }
    
    return objp;
}

/**
 * Free object to cache
 */
void kmem_cache_free(kmem_cache_t *cachep, void *objp) {
    struct slab *slabp;
    
    if (!cachep || !objp) return;
    
    // Find which slab this object belongs to
    slabp = (struct slab *)((unsigned long)objp & ~(PAGE_SIZE - 1));
    
    // Validate slab
    if (slabp->magic != SLAB_RED_MAGIC1 || slabp->slab_cache != cachep) {
        panic("Invalid object passed to kmem_cache_free");
        return;
    }
    
    // Poison object if debugging enabled
    if (cachep->flags & SLAB_POISON) {
        memset(objp, SLAB_POISON_BYTE, cachep->object_size);
    }
    
    spin_lock(&cache_chain_lock);
    
    // Add object to freelist
    *((void **)objp) = slabp->freelist;
    slabp->freelist = objp;
    slabp->inuse--;
    
    // Move slab to appropriate list
    if (slabp->inuse == 0) {
        list_del(&slabp->list);
        list_add(&slabp->list, &cachep->slabs_free);
    } else if (slabp->inuse == cachep->num - 1) {
        list_del(&slabp->list);
        list_add(&slabp->list, &cachep->slabs_partial);
    }
    
    // Update statistics
    cachep->stats.active--;
    
    spin_unlock(&cache_chain_lock);
}

/**
 * Bulk allocation for performance
 */
int kmem_cache_alloc_bulk(kmem_cache_t *cachep, unsigned long flags,
                         size_t size, void **p) {
    size_t i;
    
    for (i = 0; i < size; i++) {
        p[i] = kmem_cache_alloc(cachep, flags);
        if (!p[i]) {
            // Free already allocated objects
            while (i--) {
                kmem_cache_free(cachep, p[i]);
            }
            return i;
        }
    }
    
    return size;
}

/**
 * Bulk free for performance
 */
void kmem_cache_free_bulk(kmem_cache_t *cachep, size_t size, void **p) {
    for (size_t i = 0; i < size; i++) {
        kmem_cache_free(cachep, p[i]);
    }
}

/**
 * Get cache size
 */
unsigned int kmem_cache_size(kmem_cache_t *cachep) {
    return cachep ? cachep->object_size : 0;
}

/**
 * Get cache name
 */
const char* kmem_cache_name(kmem_cache_t *cachep) {
    return cachep ? cachep->name : "unknown";
}

/**
 * Print cache information
 */
void kmem_cache_info(kmem_cache_t *cachep) {
    if (!cachep) return;
    
    screen_print("Cache: ");
    screen_print(cachep->name);
    screen_print("\n  Object size: ");
    screen_print_dec(cachep->object_size);
    screen_print("\n  Objects per slab: ");
    screen_print_dec(cachep->num);
    screen_print("\n  Active objects: ");
    screen_print_dec(cachep->stats.active);
    screen_print("\n  Max active: ");
    screen_print_dec(cachep->stats.max_active);
    screen_print("\n  Total allocated: ");
    screen_print_dec(cachep->stats.allocated);
    screen_print("\n  Total freed: ");
    screen_print_dec(cachep->stats.freed);
    screen_print("\n  Errors: ");
    screen_print_dec(cachep->stats.errors);
    screen_print("\n");
}

/**
 * Initialize SLAB allocator
 */
void slab_init(void) {
    debug_print(DEBUG_INFO, "Initializing Linux-inspired SLAB allocator");
    
    // Initialize cache chain
    INIT_LIST_HEAD(&cache_chain);
    
    // Create common size classes for kmalloc
    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
    
    for (int i = 0; i < 12; i++) {
        char name[16];
        sprintf(name, "kmalloc-%d", sizes[i]);
        kmalloc_caches[i] = kmem_cache_create(name, sizes[i], 0, 
                                            SLAB_HWCACHE_ALIGN, NULL, NULL);
        if (!kmalloc_caches[i]) {
            panic("Failed to create kmalloc cache");
        }
    }
    
    debug_print(DEBUG_INFO, "SLAB allocator initialized successfully");
}

/**
 * Initialize common kernel caches
 */
void kmem_cache_init(void) {
    // Initialize SLAB allocator first
    slab_init();
    
    // Create common kernel caches
    vm_area_cache = kmem_cache_create("vm_area_struct", sizeof(struct vm_area_struct), 
                                      0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    
    files_cache = kmem_cache_create("files_struct", sizeof(struct files_struct), 
                                    0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    
    mm_struct_cache = kmem_cache_create("mm_struct", sizeof(struct mm_struct), 
                                       0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    
    inode_cache = kmem_cache_create("inode", sizeof(struct inode), 
                                    0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    
    dentry_cache = kmem_cache_create("dentry", sizeof(struct dentry), 
                                     0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    
    names_cache = kmem_cache_create("names_cache", 256, 
                                    0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    
    pid_cache = kmem_cache_create("pid", sizeof(struct pid), 
                                  0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    
    debug_print(DEBUG_INFO, "Common kernel caches initialized");
}

/**
 * Print SLAB debug information
 */
void slab_debug_info(void) {
    kmem_cache_t *cachep;
    
    screen_print("\n=== SLAB Allocator Debug Info ===\n");
    
    spin_lock(&cache_chain_lock);
    
    list_for_each_entry(cachep, &cache_chain, list) {
        kmem_cache_info(cachep);
    }
    
    spin_unlock(&cache_chain_lock);
}
