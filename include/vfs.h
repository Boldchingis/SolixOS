#ifndef SOLIX_VFS_H
#define SOLIX_VFS_H

#include "types.h"

// File types
#define FT_REGULAR 1
#define FT_DIRECTORY 2
#define FT_DEVICE 3

// File permissions
#define PERM_READ 0x100
#define PERM_WRITE 0x200
#define PERM_EXEC 0x400

// File flags
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_CREAT 0x0040
#define O_TRUNC 0x0200

// Seek origins
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// Superblock structure
typedef struct superblock {
    uint32_t magic;           // Filesystem magic number
    uint32_t version;         // Filesystem version
    uint32_t block_size;      // Block size in bytes
    uint32_t total_blocks;    // Total number of blocks
    uint32_t free_blocks;     // Number of free blocks
    uint32_t inode_count;     // Total number of inodes
    uint32_t free_inodes;     // Number of free inodes
    uint32_t inode_table;     // Block number of inode table
    uint32_t data_blocks;     // Starting block of data area
    uint32_t bitmap_blocks;   // Number of bitmap blocks
} __attribute__((packed)) superblock_t;

// Inode structure
typedef struct inode {
    uint32_t mode;            // File type and permissions
    uint32_t uid;             // User ID
    uint32_t gid;             // Group ID
    uint32_t size;            // File size in bytes
    uint32_t atime;           // Last access time
    uint32_t mtime;           // Last modification time
    uint32_t ctime;           // Creation time
    uint32_t links;           // Number of hard links
    uint32_t blocks;          // Number of blocks allocated
    uint32_t direct[12];      // Direct block pointers
    uint32_t indirect;        // Single indirect block
    uint32_t double_indirect; // Double indirect block
} __attribute__((packed)) inode_t;

// Directory entry structure
typedef struct dir_entry {
    uint32_t inode;           // Inode number
    char name[256];           // Filename
} __attribute__((packed)) dir_entry_t;

// File operations structure
typedef struct file_ops {
    ssize_t (*read)(void* private_data, void* buffer, size_t count);
    ssize_t (*write)(void* private_data, const void* buffer, size_t count);
    int (*seek)(void* private_data, uint32_t offset, int whence);
    int (*ioctl)(void* private_data, uint32_t request, void* arg);
    int (*close)(void* private_data);
} file_ops_t;

// VFS node structure
typedef struct vnode {
    uint32_t inode_num;
    inode_t* inode;
    struct vnode* parent;
    struct vnode* children;
    struct vnode* next;
    file_ops_t* ops;
    void* private_data;
} vnode_t;

// File structure (for open files)
typedef struct file {
    vnode_t* vnode;
    uint32_t offset;
    uint32_t flags;
    uint32_t ref_count;
} file_t;

// VFS functions
void vfs_init(void);
int vfs_mount(const char* device, const char* mountpoint);
int vfs_umount(const char* mountpoint);

// File operations
int vfs_open(const char* pathname, uint32_t flags);
int vfs_close(int fd);
ssize_t vfs_read(int fd, void* buffer, size_t count);
ssize_t vfs_write(int fd, const void* buffer, size_t count);
int vfs_seek(int fd, uint32_t offset, int whence);
int vfs_ioctl(int fd, uint32_t request, void* arg);

// Directory operations
int vfs_mkdir(const char* pathname);
int vfs_rmdir(const char* pathname);
int vfs_readdir(const char* pathname, dir_entry_t* entries, int count);

// File operations
int vfs_create(const char* pathname, uint32_t mode);
int vfs_unlink(const char* pathname);
int vfs_stat(const char* pathname, inode_t* stat);

#endif
