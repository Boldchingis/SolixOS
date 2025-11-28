#ifndef SOLIX_FS_MODERN_H
#define SOLIX_FS_MODERN_H

#include "types.h"
#include "kernel.h"
#include "vfs.h"
#include "linux_vfs.h"

/**
 * SolixOS Modern Filesystem Layer
 * Enhanced VFS with journaling, snapshots, and modern filesystem support
 * Version: 2.0
 */

// Modern filesystem types
#define FS_TYPE_EXT4     0xEF53
#define FS_TYPE_XFS      0x58465342
#define FS_TYPE_BTRFS    0x9123683E
#define FS_TYPE_F2FS     0xF2F52010
#define FS_TYPE_ZFS      0x2FC12FC1
#define FS_TYPE_APFS     0x42443331
#define FS_TYPE_NTFS     0x5346544E

// Journaling support
#define JOURNAL_HAS_COMPAT_FEATURES 0x0000
#define JOURNAL_HAS_INCOMPAT_FEATURES 0x0001
#define JOURNAL_HAS_RO_COMPAT_FEATURES 0x0002

#define JOURNAL_FEATURE_COMPAT_CHECKSUM 0x0001
#define JOURNAL_FEATURE_INCOMPAT_REVOKE 0x0001
#define JOURNAL_FEATURE_INCOMPAT_ASYNC_COMMIT 0x0002
#define JOURNAL_FEATURE_INCOMPAT_CSUM_V2 0x0004
#define JOURNAL_FEATURE_INCOMPAT_64BIT 0x0008

// Snapshot support
#define SNAPSHOT_MAX_COUNT 256
#define SNAPSHOT_MAX_SIZE 1024 * 1024 * 1024  // 1GB per snapshot
#define SNAPSHOT_NAME_MAX 256

// Copy-on-Write (CoW) support
#define COW_BLOCK_SIZE 4096
#define COW_MAX_REFERENCES 1024
#define COW_HASH_SIZE 65536

// Filesystem quotas
#define QUOTA_MAX_USERS 65536
#define QUOTA_MAX_GROUPS 65536
#define QUOTA_BLOCK_SIZE 1024
#define QUOTA_INODE_SIZE 256

// Access Control Lists (ACLs)
#define ACL_MAX_ENTRIES 1024
#define ACL_USER_OBJ 0x01
#define ACL_USER 0x02
#define ACL_GROUP_OBJ 0x04
#define ACL_GROUP 0x08
#define ACL_MASK 0x10
#define ACL_OTHER 0x20

// Extended attributes
#define XATTR_MAX_SIZE 65536
#define XATTR_MAX_NAME_LEN 255
#define XATTR_SECURITY_PREFIX "security."
#define XATTR_USER_PREFIX "user."
#define XATTR_SYSTEM_PREFIX "system."

// File encryption
#define ENCRYPTION_KEY_SIZE 32
#define ENCRYPTION_IV_SIZE 16
#define ENCRYPTION_BLOCK_SIZE 16
#define ENCRYPTION_AES_XTS 1
#define ENCRYPTION_CHACHA20 2

// Filesystem journal structure
typedef struct journal_superblock {
    uint32_t magic;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t first_block;
    uint32_t last_block;
    uint32_t sequence;
    uint32_t features;
    uint32_t compat_features;
    uint32_t incompat_features;
    uint32_t ro_compat_features;
    uint8_t uuid[16];
    uint32_t checksum_type;
    uint32_t checksum_seed;
    uint8_t padding[440];
    uint32_t checksum;
} __packed journal_superblock_t;

// Journal block structure
typedef struct journal_block {
    uint32_t sequence;
    uint32_t checksum;
    uint32_t flags;
    uint32_t block_type;
    uint64_t timestamp;
    uint8_t data[4096 - 32];
} __packed journal_block_t;

// Snapshot structure
typedef struct snapshot {
    uint32_t id;
    char name[SNAPSHOT_NAME_MAX];
    uint64_t timestamp;
    uint64_t size;
    uint32_t block_count;
    uint32_t* block_map;
    bool readonly;
    bool active;
    struct snapshot* next;
} snapshot_t;

// CoW block structure
typedef struct cow_block {
    uint64_t physical_address;
    uint64_t logical_address;
    uint32_t reference_count;
    uint32_t checksum;
    uint8_t* data;
    struct cow_block* next;
} cow_block_t;

// Quota structure
typedef struct quota {
    uint32_t uid;
    uint32_t gid;
    uint64_t space_used;
    uint64_t space_limit;
    uint32_t inodes_used;
    uint32_t inodes_limit;
    uint64_t grace_time;
    bool active;
} quota_t;

// ACL entry structure
typedef struct acl_entry {
    uint16_t tag;
    uint16_t perm;
    uint32_t id;
    uint32_t flags;
} acl_entry_t;

// Extended attribute structure
typedef struct xattr {
    char name[XATTR_MAX_NAME_LEN + 1];
    void* value;
    uint32_t value_len;
    uint32_t flags;
    struct xattr* next;
} xattr_t;

// File encryption structure
typedef struct file_encryption {
    uint32_t algorithm;
    uint8_t key[ENCRYPTION_KEY_SIZE];
    uint8_t iv[ENCRYPTION_IV_SIZE];
    uint32_t key_version;
    bool encrypted;
} file_encryption_t;

// Modern inode structure
typedef struct inode_modern {
    uint32_t ino;
    uint16_t mode;
    uint16_t uid;
    uint16_t gid;
    uint32_t size;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    uint32_t blksize;
    uint32_t blocks;
    
    // Modern features
    uint64_t version;
    uint32_t generation;
    uint32_t flags;
    uint32_t extra_isize;
    
    // Journaling
    uint32_t journal_seq;
    uint32_t transaction_id;
    
    // Snapshots
    uint32_t snapshot_id;
    bool cow_enabled;
    
    // Quotas
    uint32_t quota_blocks;
    uint32_t quota_inodes;
    
    // ACLs
    acl_entry_t* acl;
    uint32_t acl_count;
    
    // Extended attributes
    xattr_t* xattr;
    uint32_t xattr_count;
    
    // Encryption
    file_encryption_t encryption;
    
    // Block pointers
    uint32_t* direct_blocks;
    uint32_t* indirect_blocks;
    uint32_t* double_indirect;
    uint32_t* triple_indirect;
    
    // Hash for integrity checking
    uint32_t checksum;
} inode_modern_t;

// Modern superblock structure
typedef struct superblock_modern {
    uint32_t magic;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t free_blocks;
    uint32_t inode_count;
    uint32_t free_inodes;
    uint32_t first_inode;
    
    // Modern features
    uint32_t features;
    uint32_t compat_features;
    uint32_t incompat_features;
    uint32_t ro_compat_features;
    
    // Journaling
    uint64_t journal_start;
    uint32_t journal_blocks;
    journal_superblock_t* journal_sb;
    
    // Snapshots
    uint32_t snapshot_count;
    snapshot_t* snapshots;
    
    // CoW
    cow_block_t* cow_blocks;
    uint32_t cow_block_count;
    
    // Quotas
    quota_t* user_quotas;
    quota_t* group_quotas;
    
    // Checksums and integrity
    uint32_t checksum_seed;
    uint32_t checksum_type;
    
    // Encryption
    uint8_t master_key[ENCRYPTION_KEY_SIZE];
    
    // UUID and labels
    uint8_t uuid[16];
    char volume_label[256];
    char mount_point[256];
    
    // Statistics
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t cache_hits;
    uint64_t cache_misses;
} superblock_modern_t;

// Modern filesystem operations
struct filesystem_ops_modern {
    int (*mount)(const char* device, const char* mountpoint, uint32_t flags);
    int (*unmount)(const char* mountpoint);
    int (*statfs)(const char* path, struct statfs* buf);
    
    // Journaling operations
    int (*journal_start)(void);
    int (*journal_commit)(void);
    int (*journal_abort)(void);
    int (*journal_recover)(void);
    
    // Snapshot operations
    int (*snapshot_create)(const char* name);
    int (*snapshot_delete)(const char* name);
    int (*snapshot_rollback)(const char* name);
    int (*snapshot_list)(void);
    
    // CoW operations
    int (*cow_enable)(inode_modern_t* inode);
    int (*cow_disable)(inode_modern_t* inode);
    int (*cow_clone)(inode_modern_t* src, inode_modern_t* dst);
    
    // Quota operations
    int (*quota_set)(uint32_t uid, uint32_t gid, uint64_t space_limit, uint32_t inode_limit);
    int (*quota_get)(uint32_t uid, uint32_t gid, quota_t* quota);
    int (*quota_enable)(void);
    int (*quota_disable)(void);
    
    // ACL operations
    int (*acl_set)(inode_modern_t* inode, acl_entry_t* acl, uint32_t count);
    int (*acl_get)(inode_modern_t* inode, acl_entry_t** acl, uint32_t* count);
    int (*acl_remove)(inode_modern_t* inode);
    
    // Extended attribute operations
    int (*xattr_set)(inode_modern_t* inode, const char* name, void* value, uint32_t len);
    int (*xattr_get)(inode_modern_t* inode, const char* name, void* value, uint32_t* len);
    int (*xattr_remove)(inode_modern_t* inode, const char* name);
    int (*xattr_list)(inode_modern_t* inode, char* names, uint32_t* len);
    
    // Encryption operations
    int (*encrypt_file)(inode_modern_t* inode, uint8_t* key);
    int (*decrypt_file)(inode_modern_t* inode, uint8_t* key);
    int (*change_key)(inode_modern_t* inode, uint8_t* old_key, uint8_t* new_key);
};

// Modern file structure
typedef struct file_modern {
    inode_modern_t* inode;
    uint32_t pos;
    uint32_t flags;
    uint32_t mode;
    
    // Modern features
    uint32_t access_time;
    uint32_t modification_time;
    uint32_t change_time;
    
    // Cache
    uint8_t* cache;
    uint32_t cache_size;
    uint32_t cache_pos;
    bool cache_dirty;
    
    // Journaling
    uint32_t journal_seq;
    bool journal_active;
    
    // CoW
    bool cow_enabled;
    cow_block_t* cow_blocks;
    
    // Encryption
    file_encryption_t* encryption;
    uint8_t* decrypt_buffer;
} file_modern_t;

// Modern VFS functions
API int vfs_modern_init(void);
API int vfs_modern_mount(const char* device, const char* mountpoint, const char* fstype, uint32_t flags);
API int vfs_modern_unmount(const char* mountpoint);
API int vfs_modern_statfs(const char* path, struct statfs* buf);

// Journaling functions
API int journal_start_transaction(void);
API int journal_commit_transaction(void);
API int journal_abort_transaction(void);
API int journal_recover(const char* device);

// Snapshot functions
API int snapshot_create(const char* mountpoint, const char* name);
API int snapshot_delete(const char* mountpoint, const char* name);
API int snapshot_rollback(const char* mountpoint, const char* name);
API int snapshot_list(const char* mountpoint);

// CoW functions
API int cow_enable_file(const char* path);
API int cow_disable_file(const char* path);
API int cow_clone_file(const char* src, const char* dst);

// Quota functions
API int quota_set_user(const char* mountpoint, uint32_t uid, uint64_t space_limit, uint32_t inode_limit);
API int quota_set_group(const char* mountpoint, uint32_t gid, uint64_t space_limit, uint32_t inode_limit);
API int quota_get_user(const char* mountpoint, uint32_t uid, quota_t* quota);
API int quota_get_group(const char* mountpoint, uint32_t gid, quota_t* quota);

// ACL functions
API int acl_set_file(const char* path, acl_entry_t* acl, uint32_t count);
API int acl_get_file(const char* path, acl_entry_t** acl, uint32_t* count);
API int acl_remove_file(const char* path);

// Extended attribute functions
API int xattr_set(const char* path, const char* name, void* value, uint32_t len);
API int xattr_get(const char* path, const char* name, void* value, uint32_t* len);
API int xattr_remove(const char* path, const char* name);
API int xattr_list(const char* path, char* names, uint32_t* len);

// Encryption functions
API int encrypt_file(const char* path, uint8_t* key);
API int decrypt_file(const char* path, uint8_t* key);
API int change_file_key(const char* path, uint8_t* old_key, uint8_t* new_key);

// Modern filesystem registration
API int fs_register_modern(const char* name, struct filesystem_ops_modern* ops);
API int fs_unregister_modern(const char* name);

#endif
