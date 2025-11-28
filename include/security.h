#ifndef SOLIX_SECURITY_H
#define SOLIX_SECURITY_H

#include "types.h"
#include "kernel.h"

/**
 * SolixOS Security Framework
 * Modern security with capabilities, permissions, and access control
 * Version: 2.0
 */

// Security capability flags
#define CAP_CHOWN        (1 << 0)  // Change file ownership
#define CAP_DAC_OVERRIDE (1 << 1)  // Override discretionary access control
#define CAP_DAC_READ_SEARCH (1 << 2)  // Read all files and search directories
#define CAP_FOWNER       (1 << 3)  // Override file ownership checks
#define CAP_FSETID       (1 << 4)  // Set file capabilities
#define CAP_KILL         (1 << 5)  // Send signals to any process
#define CAP_SETGID       (1 << 6)  // Set group ID
#define CAP_SETUID       (1 << 7)  // Set user ID
#define CAP_SETPCAP      (1 << 8)  // Set process capabilities
#define CAP_LINUX_IMMUTABLE (1 << 9)  // Modify immutable files
#define CAP_NET_BIND_SERVICE (1 << 10)  // Bind to privileged ports
#define CAP_NET_BROADCAST (1 << 11)  // Send broadcast packets
#define CAP_NET_ADMIN    (1 << 12)  // Administer network interfaces
#define CAP_NET_RAW      (1 << 13)  // Use raw sockets
#define CAP_IPC_LOCK     (1 << 14)  // Lock shared memory
#define CAP_IPC_OWNER    (1 << 15)  // Override IPC ownership checks
#define CAP_SYS_MODULE   (1 << 16)  // Load/unload kernel modules
#define CAP_SYS_RAWIO    (1 << 17)  // Direct hardware access
#define CAP_SYS_CHROOT   (1 << 18)  // Use chroot
#define CAP_SYS_PTRACE   (1 << 19)  // Trace any process
#define CAP_SYS_PACCT    (1 << 20)  // Process accounting
#define CAP_SYS_ADMIN    (1 << 21)  // System administration
#define CAP_SYS_BOOT     (1 << 22)  // Reboot system
#define CAP_SYS_NICE     (1 << 23)  // Change process priority
#define CAP_SYS_RESOURCE (1 << 24)  // Override resource limits
#define CAP_SYS_TIME     (1 << 25)  // Set system clock
#define CAP_SYS_TTY_CONFIG (1 << 26)  // Configure terminal devices
#define CAP_MKNOD        (1 << 27)  // Create device files
#define CAP_LEASE        (1 << 28)  // Establish file leases
#define CAP_AUDIT_WRITE  (1 << 29)  // Write audit logs
#define CAP_AUDIT_CONTROL (1 << 30)  // Configure audit subsystem
#define CAP_SETFCAP      (1 << 31)  // Set file capabilities

// File permission bits
#define PERM_OTH_READ    0x004
#define PERM_OTH_WRITE   0x002
#define PERM_OTH_EXEC    0x001
#define PERM_GRP_READ    0x040
#define PERM_GRP_WRITE   0x020
#define PERM_GRP_EXEC    0x010
#define PERM_USR_READ    0x400
#define PERM_USR_WRITE   0x200
#define PERM_USR_EXEC    0x100
#define PERM_SETUID      0x800
#define PERM_SETGID      0x4000
#define PERM_STICKY      0x2000

// Security context structure
typedef struct security_context {
    uint32_t uid;                 // User ID
    uint32_t gid;                 // Group ID
    uint32_t euid;                // Effective user ID
    uint32_t egid;                // Effective group ID
    uint32_t groups[MAX_GROUPS];   // Supplementary groups
    uint32_t group_count;         // Number of supplementary groups
    uint32_t capabilities[CAPABILITY_COUNT / 32];  // Capability set
    uint32_t fsuid;               // Filesystem user ID
    uint32_t fsgid;               // Filesystem group ID
    bool secure;                  // Secure computing mode
} security_context_t;

// Access control list entry
typedef struct acl_entry {
    uint32_t type;                // ACL type
    uint32_t tag;                 // ACL tag (user, group, other)
    uint32_t id;                  // User or group ID
    uint32_t perms;               // Permissions
} acl_entry_t;

// File security metadata
typedef struct file_security {
    uint32_t mode;                // File mode and permissions
    uint32_t uid;                 // Owner user ID
    uint32_t gid;                 // Owner group ID
    acl_entry_t* acl;             // Access control list
    uint32_t acl_count;           // Number of ACL entries
    uint32_t capabilities[CAPABILITY_COUNT / 32];  // File capabilities
} file_security_t;

// Security check functions
API bool security_check_permission(security_context_t* ctx, file_security_t* fs, uint32_t perm);
API bool security_check_capability(security_context_t* ctx, uint32_t cap);
API bool security_check_file_access(security_context_t* ctx, const char* path, uint32_t flags);

// Security context management
API void security_init_context(security_context_t* ctx);
API void security_set_root(security_context_t* ctx);
API void security_set_user(security_context_t* ctx, uint32_t uid, uint32_t gid);
API void security_add_group(security_context_t* ctx, uint32_t gid);
API void security_set_capability(security_context_t* ctx, uint32_t cap);
API void security_clear_capability(security_context_t* ctx, uint32_t cap);

// File security operations
API void security_init_file_security(file_security_t* fs);
API void security_set_file_mode(file_security_t* fs, uint32_t mode);
API void security_set_file_owner(file_security_t* fs, uint32_t uid, uint32_t gid);
API int security_add_acl_entry(file_security_t* fs, acl_entry_t* entry);

// Audit and logging
API void security_log_event(const char* event, security_context_t* ctx);
API void security_log_access(const char* path, security_context_t* ctx, uint32_t result);

// System security initialization
API void security_init_system(void);
API void security_init_process(security_context_t* ctx, security_context_t* parent);

#endif
