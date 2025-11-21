#ifndef SOLIX_MODULE_H
#define SOLIX_MODULE_H

#include "types.h"
#include "kernel.h"

/**
 * Linux-Inspired Module System for SolixOS
 * Dynamic kernel module loading and unloading
 * Based on Linux module design principles
 */

// Module flags
#define MODULE_FLAG_LOADED      0x01
#define MODULE_FLAG_INITIALIZED 0x02
#define MODULE_FLAG_GOING       0x04
#define MODULE_FLAG_COMING      0x08
#define MODULE_FLAG_FORMER      0x10

// Module state
#define MODULE_STATE_LIVE       0
#define MODULE_STATE_COMING     1
#define MODULE_STATE_GOING      2
#define MODULE_STATE_UNFORMED   3

// Module license types
#define MODULE_LICENSE_GPL      "GPL"
#define MODULE_LICENSE_GPL_V2    "GPL v2"
#define MODULE_LICENSE_GPL_V3    "GPL v3"
#define MODULE_LICENSE_LGPL      "LGPL"
#define MODULE_LICENSE_BSD       "BSD"
#define MODULE_LICENSE_MIT       "MIT"
#define MODULE_LICENSE_PROPRIETARY "Proprietary"

// Module author information
struct module_author {
    char name[64];
    char email[64];
};

// Module version information
struct module_version {
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    char version_string[32];
};

// Module alias structure
struct module_alias {
    char alias[256];
    char module_name[64];
    struct module_alias *next;
};

// Module parameter structure
struct module_param {
    char name[64];
    char type[16];
    void *value;
    char description[256];
    struct module_param *next;
};

// Module symbol structure
struct module_symbol {
    char name[256];
    void *value;
    uint32_t size;
    struct module_symbol *next;
};

// Module dependency structure
struct module_dependency {
    char module_name[64];
    uint32_t version_required;
    struct module_dependency *next;
};

// Module structure
struct module {
    // Basic information
    char name[64];                    // Module name
    char version[32];                 // Module version
    char license[32];                 // Module license
    char description[256];            // Module description
    
    // Module state and flags
    uint32_t state;                   // Current module state
    uint32_t flags;                   // Module flags
    
    // Module size and location
    void *module_base;                // Base address of module
    uint32_t module_size;             // Size of module in memory
    void *init_text;                  // Initialization code section
    void *exit_text;                  // Cleanup code section
    
    // Module sections
    void *text_section;               // Code section
    uint32_t text_size;               // Code section size
    void *data_section;               // Data section
    uint32_t data_size;               // Data section size
    void *rodata_section;             // Read-only data section
    uint32_t rodata_size;             // Read-only data section size
    void *bss_section;                // BSS section
    uint32_t bss_size;                // BSS section size
    
    // Module entry points
    int (*init)(void);                // Module initialization function
    void (*exit)(void);               // Module cleanup function
    
    // Module metadata
    struct module_author *authors;   // List of authors
    struct module_alias *aliases;    // List of aliases
    struct module_param *parameters; // List of parameters
    struct module_symbol *symbols;   // List of exported symbols
    struct module_dependency *deps;   // List of dependencies
    
    // Module statistics
    uint32_t refcount;                // Reference count
    uint32_t use_count;               // Use count
    uint32_t init_size;               // Size of init section
    uint32_t core_size;               // Size of core sections
    
    // Module timing
    uint32_t load_time;               // Time when module was loaded
    uint32_t init_time;               // Time when init was called
    
    // Module lists
    struct list_head list;             // List of all modules
    struct list_head modules_which_use_me; // Modules that depend on this one
    
    // Locking
    spinlock_t lock;                  // Module lock
    
    // Module-specific data
    void *module_data;               // Module-specific private data
    
    // Module version info
    struct module_version version_info;
    
    // Module attributes
    char *modinfo_attrs;              // Module information attributes
    
    // Module signature (for security)
    uint8_t signature[256];          // Module signature
    uint32_t signature_len;          // Signature length
    bool signed;                      // Whether module is signed
    
    // Module hash
    uint32_t crc;                     // CRC32 of module
    uint32_t sha256[8];              // SHA256 hash of module
};

// Module parameter types
#define MODULE_PARAM_TYPE_BYTE    1
#define MODULE_PARAM_TYPE_SHORT   2
#define MODULE_PARAM_TYPE_INT     3
#define MODULE_PARAM_TYPE_LONG    4
#define MODULE_PARAM_TYPE_UINT    5
#define MODULE_PARAM_TYPE_ULONG   6
#define MODULE_PARAM_TYPE_STRING  7
#define MODULE_PARAM_TYPE_BOOL    8
#define MODULE_PARAM_TYPE_INVBOOL 9

// Module parameter macros
#define module_param(name, type, perm) \
    static struct module_param __module_param_##name = { \
        .name = #name, \
        .type = #type, \
        .value = &name, \
        .description = "Module parameter " #name \
    }

#define module_param_named(name, value, type, perm) \
    static struct module_param __module_param_##name = { \
        .name = #name, \
        .type = #type, \
        .value = &value, \
        .description = "Module parameter " #name \
    }

#define module_param_string(name, size, perm) \
    static char name[size]; \
    module_param_named(name, name, string, perm)

#define module_param_array(name, type, nump, perm) \
    static type name[nump]; \
    static int __module_param_##name##_num = nump; \
    module_param_named(name, name, array, perm)

// Module information macros
#define MODULE_AUTHOR(name) \
    static struct module_author __module_author_##__LINE__ = { \
        .name = name \
    }; \
    static void __register_module_author_##__LINE__(void) __attribute__((constructor)); \
    static void __register_module_author_##__LINE__(void) { \
        register_module_author(&__module_author_##__LINE__); \
    }

#define MODULE_DESCRIPTION(desc) \
    static const char __module_description[] = desc; \
    static void __register_module_description_##__LINE__(void) __attribute__((constructor)); \
    static void __register_module_description_##__LINE__(void) { \
        register_module_description(__module_description); \
    }

#define MODULE_LICENSE(license) \
    static const char __module_license[] = license; \
    static void __register_module_license_##__LINE__(void) __attribute__((constructor)); \
    static void __register_module_license_##__LINE__(void) { \
        register_module_license(__module_license); \
    }

#define MODULE_VERSION(version) \
    static const char __module_version[] = version; \
    static void __register_module_version_##__LINE__(void) __attribute__((constructor)); \
    static void __register_module_version_##__LINE__(void) { \
        register_module_version(__module_version); \
    }

#define MODULE_ALIAS(alias) \
    static struct module_alias __module_alias_##__LINE__ = { \
        .alias = alias \
    }; \
    static void __register_module_alias_##__LINE__(void) __attribute__((constructor)); \
    static void __register_module_alias_##__LINE__(void) { \
        register_module_alias(&__module_alias_##__LINE__); \
    }

#define MODULE_DEVICE_TABLE(table_name, table) \
    static const struct device_id __device_table_##table_name[] = table; \
    static void __register_device_table_##__LINE__(void) __attribute__((constructor)); \
    static void __register_device_table_##__LINE__(void) { \
        register_device_table(table_name, __device_table_##table_name); \
    }

// Module symbol export macros
#define EXPORT_SYMBOL(sym) \
    static struct module_symbol __module_symbol_##sym = { \
        .name = #sym, \
        .value = &sym, \
        .size = sizeof(sym) \
    }; \
    static void __export_symbol_##sym(void) __attribute__((constructor)); \
    static void __export_symbol_##sym(void) { \
        export_symbol(&__module_symbol_##sym); \
    }

#define EXPORT_SYMBOL_GPL(sym) \
    static struct module_symbol __module_symbol_gpl_##sym = { \
        .name = #sym, \
        .value = &sym, \
        .size = sizeof(sym) \
    }; \
    static void __export_symbol_gpl_##sym(void) __attribute__((constructor)); \
    static void __export_symbol_gpl_##sym(void) { \
        export_symbol_gpl(&__module_symbol_gpl_##sym); \
    }

// Module init/exit macros
#define module_init(initfn) \
    static int (*__module_init_fn)(void) = initfn; \
    static void __register_module_init_##__LINE__(void) __attribute__((constructor)); \
    static void __register_module_init_##__LINE__(void) { \
        register_module_init(__module_init_fn); \
    }

#define module_exit(exitfn) \
    static void (*__module_exit_fn)(void) = exitfn; \
    static void __register_module_exit_##__LINE__(void) __attribute__((constructor)); \
    static void __register_module_exit_##__LINE__(void) { \
        register_module_exit(__module_exit_fn); \
    }

// Module management functions
extern int module_load(const char *filename);
extern int module_unload(const char *name);
extern struct module *module_find(const char *name);
extern int module_refcount_inc(struct module *mod);
extern int module_refcount_dec(struct module *mod);

// Module parameter functions
extern int module_param_register(const char *name, const char *type, void *value, const char *desc);
extern int module_param_set(const char *name, const char *value);
extern int module_param_get(const char *name, char *value, size_t size);

// Module symbol functions
extern int export_symbol(struct module_symbol *sym);
extern int export_symbol_gpl(struct module_symbol *sym);
extern void *resolve_symbol(const char *name);
extern int list_symbols(char *buffer, size_t size);

// Module information functions
extern void register_module_author(struct module_author *author);
extern void register_module_description(const char *desc);
extern void register_module_license(const char *license);
extern void register_module_version(const char *version);
extern void register_module_alias(struct module_alias *alias);
extern void register_device_table(const char *table_name, const void *table);

// Module initialization functions
extern void register_module_init(int (*init_fn)(void));
extern void register_module_exit(void (*exit_fn)(void));

// Module dependency functions
extern int module_add_dependency(struct module *mod, const char *dep_name);
extern int module_check_dependencies(struct module *mod);
extern int module_resolve_dependencies(struct module *mod);

// Module utility functions
extern uint32_t module_calculate_crc(const void *data, size_t size);
extern void module_calculate_sha256(const void *data, size_t size, uint32_t *hash);
extern int module_verify_signature(struct module *mod);
extern int module_verify_integrity(struct module *mod);

// Module listing and information
extern int list_modules(char *buffer, size_t size);
extern int get_module_info(const char *name, char *buffer, size_t size);
extern void print_module_info(struct module *mod);

// Module statistics
extern struct {
    uint32_t total_modules;
    uint32_t loaded_modules;
    uint32_t failed_loads;
    uint32_t total_symbols;
    uint32_t exported_symbols;
    uint32_t total_dependencies;
} module_stats;

// Module initialization
extern void module_init_subsystem(void);
extern void module_cleanup_subsystem(void);

// Module configuration
#define CONFIG_MODULES
#define CONFIG_MODULE_UNLOAD
#define CONFIG_MODULE_FORCE_LOAD
#define CONFIG_MODULE_SIG
#define CONFIG_MODULE_COMPRESS
#define CONFIG_MODULE_STRIP

// Module debugging
#define MODULE_DEBUG
#define MODULE_DEBUG_LOAD
#define MODULE_DEBUG_UNLOAD
#define MODULE_DEBUG_SYMBOLS

// Helper macros
#define THIS_MODULE (&__this_module)
#define MODULE_NAME(x) __stringify(x)

// External module reference (for current module)
extern struct module __this_module;

// Module error codes
#define MODULE_SUCCESS           0
#define MODULE_ERROR_INVALID_ARG -1
#define MODULE_ERROR_NOT_FOUND   -2
#define MODULE_ERROR_ALREADY_LOADED -3
#define MODULE_ERROR_DEPENDENCY -4
#define MODULE_ERROR_SIGNATURE   -5
#define MODULE_ERROR_CORRUPTED  -6
#define MODULE_ERROR_MEMORY     -7
#define MODULE_ERROR_PERMISSION -8
#define MODULE_ERROR_FORMAT     -9

#endif
