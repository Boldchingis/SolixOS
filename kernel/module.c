#include "module.h"
#include "kernel.h"
#include "mm.h"
#include "printk.h"
#include "slab.h"
#include "vfs.h"

/**
 * Linux-Inspired Module System Implementation
 * Dynamic kernel module loading and unloading
 * Based on Linux module design principles
 */

// Global module list
static LIST_HEAD(module_list);
static spinlock_t module_list_lock = SPIN_LOCK_UNLOCKED;

// Module symbol table
static LIST_HEAD(symbol_table);
static spinlock_t symbol_table_lock = SPIN_LOCK_UNLOCKED;

// Module caches
static kmem_cache_t *module_cache;
static kmem_cache_t *module_symbol_cache;
static kmem_cache_t *module_param_cache;
static kmem_cache_t *module_alias_cache;

// Module statistics
struct {
    uint32_t total_modules;
    uint32_t loaded_modules;
    uint32_t failed_loads;
    uint32_t total_symbols;
    uint32_t exported_symbols;
    uint32_t total_dependencies;
} module_stats = {0};

// Current module being loaded
static struct module *current_module = NULL;

// Module file format (simplified ELF)
#define MODULE_MAGIC 0x7F454C46  // ELF magic
#define MODULE_VERSION 1

struct module_header {
    uint32_t magic;           // ELF magic
    uint32_t version;         // Module format version
    char name[64];            // Module name
    char version[32];         // Module version
    char license[32];         // Module license
    char description[256];    // Module description
    uint32_t text_size;       // Text section size
    uint32_t data_size;       // Data section size
    uint32_t rodata_size;     // Read-only data size
    uint32_t bss_size;        // BSS section size
    uint32_t init_size;       // Init section size
    uint32_t exit_size;       // Exit section size
    uint32_t symbol_count;    // Number of symbols
    uint32_t param_count;     // Number of parameters
    uint32_t alias_count;     // Number of aliases
    uint32_t dep_count;       // Number of dependencies
    uint32_t crc;             // CRC32 of module
    uint32_t signature_len;   // Signature length
    uint8_t signature[256];   // Module signature
};

/**
 * Initialize module subsystem
 */
void module_init_subsystem(void) {
    pr_info("Initializing Linux-inspired module subsystem\n");
    
    // Create caches
    module_cache = kmem_cache_create("module_cache", sizeof(struct module),
                                    0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!module_cache) {
        pr_err("Failed to create module cache\n");
        return;
    }
    
    module_symbol_cache = kmem_cache_create("module_symbol_cache", sizeof(struct module_symbol),
                                          0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!module_symbol_cache) {
        pr_err("Failed to create module symbol cache\n");
        goto err_module_cache;
    }
    
    module_param_cache = kmem_cache_create("module_param_cache", sizeof(struct module_param),
                                          0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!module_param_cache) {
        pr_err("Failed to create module parameter cache\n");
        goto err_symbol_cache;
    }
    
    module_alias_cache = kmem_cache_create("module_alias_cache", sizeof(struct module_alias),
                                         0, SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!module_alias_cache) {
        pr_err("Failed to create module alias cache\n");
        goto err_param_cache;
    }
    
    // Initialize lists
    INIT_LIST_HEAD(&module_list);
    INIT_LIST_HEAD(&symbol_table);
    
    pr_info("Module subsystem initialized successfully\n");
    return;
    
err_param_cache:
    kmem_cache_destroy(module_param_cache);
err_symbol_cache:
    kmem_cache_destroy(module_symbol_cache);
err_module_cache:
    kmem_cache_destroy(module_cache);
}

/**
 * Cleanup module subsystem
 */
void module_cleanup_subsystem(void) {
    // Unload all modules
    struct module *mod, *tmp;
    
    list_for_each_entry_safe(mod, tmp, &module_list, list) {
        if (strcmp(mod->name, "kernel") != 0) {
            module_unload(mod->name);
        }
    }
    
    // Destroy caches
    if (module_cache) kmem_cache_destroy(module_cache);
    if (module_symbol_cache) kmem_cache_destroy(module_symbol_cache);
    if (module_param_cache) kmem_cache_destroy(module_param_cache);
    if (module_alias_cache) kmem_cache_destroy(module_alias_cache);
}

/**
 * Calculate CRC32 of data
 */
uint32_t module_calculate_crc(const void *data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *bytes = (const uint8_t *)data;
    
    for (size_t i = 0; i < size; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    
    return ~crc;
}

/**
 * Calculate SHA256 hash of data
 */
void module_calculate_sha256(const void *data, size_t size, uint32_t *hash) {
    // Simplified SHA256 implementation
    // In a real kernel, this would use a proper crypto library
    memset(hash, 0, 32);
    
    // For now, just use a simple hash
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t temp = 0;
    
    for (size_t i = 0; i < size; i++) {
        temp = (temp << 8) ^ bytes[i];
        if (i % 4 == 3) {
            hash[(i / 4) % 8] = temp;
            temp = 0;
        }
    }
}

/**
 * Verify module signature
 */
int module_verify_signature(struct module *mod) {
    if (!mod) return -EINVAL;
    
    // For now, just check if signature exists
    // In a real kernel, this would verify the cryptographic signature
    if (mod->signature_len > 0) {
        mod->signed = true;
        return 0;
    }
    
    mod->signed = false;
    return 0;
}

/**
 * Verify module integrity
 */
int module_verify_integrity(struct module *mod) {
    uint32_t calculated_crc;
    
    if (!mod || !mod->module_base) return -EINVAL;
    
    // Calculate CRC of module data
    calculated_crc = module_calculate_crc(mod->module_base, mod->module_size);
    
    // Compare with stored CRC
    if (calculated_crc != mod->crc) {
        pr_err("Module %s CRC mismatch: expected 0x%08x, got 0x%08x\n",
               mod->name, mod->crc, calculated_crc);
        return -EILSEQ;
    }
    
    // Verify signature if present
    if (mod->signed) {
        int ret = module_verify_signature(mod);
        if (ret < 0) {
            return ret;
        }
    }
    
    return 0;
}

/**
 * Allocate and initialize a module structure
 */
static struct module *module_alloc(void) {
    struct module *mod;
    
    mod = kmem_cache_alloc(module_cache, GFP_KERNEL);
    if (!mod) {
        return NULL;
    }
    
    memset(mod, 0, sizeof(struct module));
    
    // Initialize fields
    mod->state = MODULE_STATE_UNFORMED;
    mod->flags = 0;
    mod->refcount = 0;
    mod->use_count = 0;
    mod->load_time = kernel_get_timestamp();
    
    spin_lock_init(&mod->lock);
    INIT_LIST_HEAD(&mod->list);
    INIT_LIST_HEAD(&mod->modules_which_use_me);
    
    return mod;
}

/**
 * Free module structure
 */
static void module_free(struct module *mod) {
    if (!mod) return;
    
    // Free sections if allocated
    if (mod->module_base) {
        kfree(mod->module_base);
    }
    
    // Free metadata
    if (mod->authors) {
        // Free author list
        struct module_author *author, *author_tmp;
        list_for_each_entry_safe(author, author_tmp, &mod->authors->list, list) {
            list_del(&author->list);
            kfree(author);
        }
    }
    
    if (mod->aliases) {
        // Free alias list
        struct module_alias *alias, *alias_tmp;
        list_for_each_entry_safe(alias, alias_tmp, &mod->aliases->list, list) {
            list_del(&alias->list);
            kmem_cache_free(module_alias_cache, alias);
        }
    }
    
    if (mod->parameters) {
        // Free parameter list
        struct module_param *param, *param_tmp;
        list_for_each_entry_safe(param, param_tmp, &mod->parameters->list, list) {
            list_del(&param->list);
            kmem_cache_free(module_param_cache, param);
        }
    }
    
    if (mod->symbols) {
        // Free symbol list
        struct module_symbol *symbol, *symbol_tmp;
        list_for_each_entry_safe(symbol, symbol_tmp, &mod->symbols->list, list) {
            list_del(&symbol->list);
            kmem_cache_free(module_symbol_cache, symbol);
        }
    }
    
    if (mod->deps) {
        // Free dependency list
        struct module_dependency *dep, *dep_tmp;
        list_for_each_entry_safe(dep, dep_tmp, &mod->deps->list, list) {
            list_del(&dep->list);
            kfree(dep);
        }
    }
    
    kmem_cache_free(module_cache, mod);
}

/**
 * Load a module from file
 */
int module_load(const char *filename) {
    struct file *file;
    struct module_header header;
    struct module *mod;
    void *module_data;
    ssize_t bytes_read;
    int ret;
    
    if (!filename) return -EINVAL;
    
    pr_info("Loading module from %s\n", filename);
    
    // Open module file
    file = filp_open(filename, O_RDONLY, 0);
    if (IS_ERR(file)) {
        ret = PTR_ERR(file);
        pr_err("Failed to open module file %s: %d\n", filename, ret);
        return ret;
    }
    
    // Read module header
    bytes_read = vfs_read(file, (char *)&header, sizeof(header), &file->f_pos);
    if (bytes_read != sizeof(header)) {
        ret = -EIO;
        pr_err("Failed to read module header: %d\n", ret);
        goto err_close_file;
    }
    
    // Verify magic and version
    if (header.magic != MODULE_MAGIC) {
        ret = -ENOEXEC;
        pr_err("Invalid module magic: 0x%08x\n", header.magic);
        goto err_close_file;
    }
    
    if (header.version != MODULE_VERSION) {
        ret = -EINVAL;
        pr_err("Unsupported module version: %d\n", header.version);
        goto err_close_file;
    }
    
    // Check if module already loaded
    if (module_find(header.name)) {
        ret = -EEXIST;
        pr_err("Module %s already loaded\n", header.name);
        goto err_close_file;
    }
    
    // Allocate module structure
    mod = module_alloc();
    if (!mod) {
        ret = -ENOMEM;
        pr_err("Failed to allocate module structure\n");
        goto err_close_file;
    }
    
    // Copy basic information
    strncpy(mod->name, header.name, sizeof(mod->name) - 1);
    strncpy(mod->version, header.version, sizeof(mod->version) - 1);
    strncpy(mod->license, header.license, sizeof(mod->license) - 1);
    strncpy(mod->description, header.description, sizeof(mod->description) - 1);
    mod->crc = header.crc;
    mod->signature_len = header.signature_len;
    memcpy(mod->signature, header.signature, sizeof(mod->signature));
    
    // Calculate total module size
    mod->module_size = sizeof(header) + header.text_size + header.data_size +
                      header.rodata_size + header.bss_size + header.init_size +
                      header.exit_size;
    
    // Allocate memory for module
    mod->module_base = kmalloc(mod->module_size, GFP_KERNEL);
    if (!mod->module_base) {
        ret = -ENOMEM;
        pr_err("Failed to allocate module memory\n");
        goto err_free_module;
    }
    
    // Copy header to module memory
    memcpy(mod->module_base, &header, sizeof(header));
    
    // Read remaining module data
    bytes_read = vfs_read(file, (char *)mod->module_base + sizeof(header),
                         mod->module_size - sizeof(header), &file->f_pos);
    if (bytes_read != mod->module_size - sizeof(header)) {
        ret = -EIO;
        pr_err("Failed to read module data: %d\n", ret);
        goto err_free_module_data;
    }
    
    // Verify module integrity
    ret = module_verify_integrity(mod);
    if (ret < 0) {
        pr_err("Module integrity verification failed: %d\n", ret);
        goto err_free_module_data;
    }
    
    // Set up section pointers
    char *module_ptr = (char *)mod->module_base + sizeof(header);
    mod->text_section = module_ptr;
    mod->text_size = header.text_size;
    module_ptr += header.text_size;
    
    mod->data_section = module_ptr;
    mod->data_size = header.data_size;
    module_ptr += header.data_size;
    
    mod->rodata_section = module_ptr;
    mod->rodata_size = header.rodata_size;
    module_ptr += header.rodata_size;
    
    mod->bss_section = module_ptr;
    mod->bss_size = header.bss_size;
    module_ptr += header.bss_size;
    
    mod->init_text = module_ptr;
    mod->init_size = header.init_size;
    module_ptr += header.init_size;
    
    mod->exit_text = module_ptr;
    mod->exit_size = header.exit_size;
    
    // Parse module metadata (symbols, parameters, etc.)
    // This is simplified - in a real kernel, this would be more complex
    ret = 0; // Assume success for now
    
    // Set module state
    mod->state = MODULE_STATE_COMING;
    mod->flags |= MODULE_FLAG_COMING;
    
    // Add to module list
    spin_lock(&module_list_lock);
    list_add(&mod->list, &module_list);
    spin_unlock(&module_list_lock);
    
    // Call module init function if available
    if (mod->init) {
        pr_debug("Calling module init function for %s\n", mod->name);
        ret = mod->init();
        if (ret < 0) {
            pr_err("Module %s init failed: %d\n", mod->name, ret);
            goto err_remove_from_list;
        }
    }
    
    // Update module state
    mod->state = MODULE_STATE_LIVE;
    mod->flags &= ~MODULE_FLAG_COMING;
    mod->flags |= MODULE_FLAG_LOADED | MODULE_FLAG_INITIALIZED;
    mod->init_time = kernel_get_timestamp();
    
    // Update statistics
    module_stats.total_modules++;
    module_stats.loaded_modules++;
    
    pr_info("Module %s loaded successfully\n", mod->name);
    
    filp_close(file, NULL);
    return 0;
    
err_remove_from_list:
    spin_lock(&module_list_lock);
    list_del(&mod->list);
    spin_unlock(&module_list_lock);
    
err_free_module_data:
    kfree(mod->module_base);
    
err_free_module:
    module_free(mod);
    
err_close_file:
    filp_close(file, NULL);
    module_stats.failed_loads++;
    
    return ret;
}

/**
 * Unload a module
 */
int module_unload(const char *name) {
    struct module *mod;
    int ret;
    
    if (!name) return -EINVAL;
    
    pr_info("Unloading module %s\n", name);
    
    // Find module
    mod = module_find(name);
    if (!mod) {
        ret = -ENOENT;
        pr_err("Module %s not found\n", name);
        return ret;
    }
    
    // Check if module can be unloaded
    if (mod->refcount > 0) {
        ret = -EBUSY;
        pr_err("Module %s is in use (refcount: %d)\n", name, mod->refcount);
        return ret;
    }
    
    // Set module state
    mod->state = MODULE_STATE_GOING;
    mod->flags |= MODULE_FLAG_GOING;
    
    // Call module exit function if available
    if (mod->exit) {
        pr_debug("Calling module exit function for %s\n", mod->name);
        mod->exit();
    }
    
    // Remove from module list
    spin_lock(&module_list_lock);
    list_del(&mod->list);
    spin_unlock(&module_list_lock);
    
    // Free module
    module_free(mod);
    
    // Update statistics
    module_stats.loaded_modules--;
    
    pr_info("Module %s unloaded successfully\n", name);
    
    return 0;
}

/**
 * Find a module by name
 */
struct module *module_find(const char *name) {
    struct module *mod;
    
    if (!name) return NULL;
    
    spin_lock(&module_list_lock);
    list_for_each_entry(mod, &module_list, list) {
        if (strcmp(mod->name, name) == 0) {
            spin_unlock(&module_list_lock);
            return mod;
        }
    }
    spin_unlock(&module_list_lock);
    
    return NULL;
}

/**
 * Increment module reference count
 */
int module_refcount_inc(struct module *mod) {
    if (!mod) return -EINVAL;
    
    spin_lock(&mod->lock);
    mod->refcount++;
    mod->use_count++;
    spin_unlock(&mod->lock);
    
    return mod->refcount;
}

/**
 * Decrement module reference count
 */
int module_refcount_dec(struct module *mod) {
    if (!mod) return -EINVAL;
    
    spin_lock(&mod->lock);
    if (mod->refcount == 0) {
        spin_unlock(&mod->lock);
        return -EINVAL;
    }
    
    mod->refcount--;
    mod->use_count--;
    spin_unlock(&mod->lock);
    
    return mod->refcount;
}

/**
 * Export a symbol
 */
int export_symbol(struct module_symbol *sym) {
    if (!sym || !sym->name || !sym->value) return -EINVAL;
    
    spin_lock(&symbol_table_lock);
    list_add(&sym->list, &symbol_table);
    spin_unlock(&symbol_table_lock);
    
    module_stats.total_symbols++;
    module_stats.exported_symbols++;
    
    pr_debug("Exported symbol: %s\n", sym->name);
    
    return 0;
}

/**
 * Export a GPL symbol
 */
int export_symbol_gpl(struct module_symbol *sym) {
    // Same as export_symbol for now
    return export_symbol(sym);
}

/**
 * Resolve a symbol
 */
void *resolve_symbol(const char *name) {
    struct module_symbol *sym;
    
    if (!name) return NULL;
    
    spin_lock(&symbol_table_lock);
    list_for_each_entry(sym, &symbol_table, list) {
        if (strcmp(sym->name, name) == 0) {
            spin_unlock(&symbol_table_lock);
            return sym->value;
        }
    }
    spin_unlock(&symbol_table_lock);
    
    return NULL;
}

/**
 * List all modules
 */
int list_modules(char *buffer, size_t size) {
    struct module *mod;
    int len = 0;
    
    if (!buffer || size == 0) return 0;
    
    spin_lock(&module_list_lock);
    
    list_for_each_entry(mod, &module_list, list) {
        int ret = snprintf(buffer + len, size - len,
                          "%-20s %s %s %d\n",
                          mod->name, mod->version, mod->license, mod->refcount);
        if (ret < 0 || len + ret >= size) break;
        len += ret;
    }
    
    spin_unlock(&module_list_lock);
    
    return len;
}

/**
 * Get module information
 */
int get_module_info(const char *name, char *buffer, size_t size) {
    struct module *mod;
    int len = 0;
    
    if (!name || !buffer || size == 0) return -EINVAL;
    
    mod = module_find(name);
    if (!mod) return -ENOENT;
    
    len = snprintf(buffer, size,
                  "Name: %s\n"
                  "Version: %s\n"
                  "License: %s\n"
                  "Description: %s\n"
                  "State: %d\n"
                  "Flags: 0x%08x\n"
                  "Size: %u bytes\n"
                  "Refcount: %d\n"
                  "Load time: %u\n"
                  "Init time: %u\n"
                  "Signed: %s\n",
                  mod->name, mod->version, mod->license, mod->description,
                  mod->state, mod->flags, mod->module_size, mod->refcount,
                  mod->load_time, mod->init_time, mod->signed ? "Yes" : "No");
    
    return len;
}

/**
 * Print module information
 */
void print_module_info(struct module *mod) {
    if (!mod) return;
    
    printk("Module: %s\n", mod->name);
    printk("  Version: %s\n", mod->version);
    printk("  License: %s\n", mod->license);
    printk("  Description: %s\n", mod->description);
    printk("  State: %d\n", mod->state);
    printk("  Flags: 0x%08x\n", mod->flags);
    printk("  Size: %u bytes\n", mod->module_size);
    printk("  Refcount: %d\n", mod->refcount);
    printk("  Load time: %u\n", mod->load_time);
    printk("  Init time: %u\n", mod->init_time);
    printk("  Signed: %s\n", mod->signed ? "Yes" : "No");
}

// Module registration functions (called by module macros)
void register_module_author(struct module_author *author) {
    if (!current_module || !author) return;
    
    // Add to module's author list
    // This is simplified - in a real kernel, this would be more complex
}

void register_module_description(const char *desc) {
    if (!current_module || !desc) return;
    
    strncpy(current_module->description, desc, sizeof(current_module->description) - 1);
}

void register_module_license(const char *license) {
    if (!current_module || !license) return;
    
    strncpy(current_module->license, license, sizeof(current_module->license) - 1);
}

void register_module_version(const char *version) {
    if (!current_module || !version) return;
    
    strncpy(current_module->version, version, sizeof(current_module->version) - 1);
}

void register_module_alias(struct module_alias *alias) {
    if (!current_module || !alias) return;
    
    // Add to module's alias list
    // This is simplified
}

void register_module_init(int (*init_fn)(void)) {
    if (!current_module) return;
    
    current_module->init = init_fn;
}

void register_module_exit(void (*exit_fn)(void)) {
    if (!current_module) return;
    
    current_module->exit = exit_fn;
}

void register_device_table(const char *table_name, const void *table) {
    // Device table registration
    // This is simplified
}

// Current module reference
struct module __this_module = {
    .name = "kernel",
    .version = "1.0.0",
    .license = "GPL",
    .description = "SolixOS Kernel",
    .state = MODULE_STATE_LIVE,
    .flags = MODULE_FLAG_LOADED | MODULE_FLAG_INITIALIZED,
};
