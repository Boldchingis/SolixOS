#include "vfs.h"
#include "mm.h"
#include "screen.h"
#include "../include/disk.h"

// VFS mount table
#define MAX_MOUNTS 16
static struct mount {
    char device[32];
    char mountpoint[256];
    vnode_t* root;
    int active;
} mount_table[MAX_MOUNTS];

// Current file table
static file_t file_table[256];
static int next_fd = 0;

// Initialize VFS
void vfs_init(void) {
    // Initialize mount table
    for (int i = 0; i < MAX_MOUNTS; i++) {
        mount_table[i].active = 0;
        mount_table[i].root = NULL;
    }
    
    // Initialize file table
    for (int i = 0; i < 256; i++) {
        file_table[i].vnode = NULL;
        file_table[i].offset = 0;
        file_table[i].flags = 0;
        file_table[i].ref_count = 0;
    }
    
    // Initialize SolixFS
    solixfs_init();
    
    // Mount root filesystem
    mount_table[0].active = 1;
    strcpy(mount_table[0].device, "hda");
    strcpy(mount_table[0].mountpoint, "/");
    mount_table[0].root = root_vnode;
    
    screen_print("VFS initialized\n");
}

// Find mount point for path
static struct mount* find_mount(const char* path) {
    struct mount* best_match = NULL;
    size_t best_len = 0;
    
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (!mount_table[i].active) continue;
        
        size_t mp_len = strlen(mount_table[i].mountpoint);
        if (strncmp(path, mount_table[i].mountpoint, mp_len) == 0) {
            if (mp_len > best_len) {
                best_len = mp_len;
                best_match = &mount_table[i];
            }
        }
    }
    
    return best_match;
}

// Resolve path to vnode
static vnode_t* resolve_path(const char* path) {
    if (path[0] == '/') {
        // Absolute path
        struct mount* mount = find_mount(path);
        if (!mount) return NULL;
        
        vnode_t* vnode = mount->root;
        const char* p = path + strlen(mount->mountpoint);
        
        while (*p && vnode) {
            // Skip leading slashes
            while (*p == '/') p++;
            
            if (!*p) break;
            
            // Extract next component
            char component[256];
            int i = 0;
            while (*p && *p != '/' && i < 255) {
                component[i++] = *p++;
            }
            component[i] = '\0';
            
            // Find in directory
            uint32_t inode_num = find_in_dir(vnode->inode_num - 1, component);
            if (inode_num == 0) return NULL;
            
            // Create vnode for this inode
            vnode_t* child = kmalloc(sizeof(vnode_t));
            child->inode_num = inode_num;
            child->inode = &inode_table[inode_num - 1];
            child->parent = vnode;
            child->children = NULL;
            child->next = NULL;
            child->ops = (child->inode->mode == FT_DIRECTORY) ? &dir_ops : &file_ops;
            child->private_data = child;
            
            vnode = child;
        }
        
        return vnode;
    } else {
        // Relative path - use current working directory
        // For now, just treat as relative to root
        char full_path[512];
        strcpy(full_path, "/");
        strcat(full_path, path);
        return resolve_path(full_path);
    }
}

// Open file
int vfs_open(const char* pathname, uint32_t flags) {
    // Find free file descriptor
    int fd = -1;
    for (int i = 0; i < 256; i++) {
        if (file_table[i].vnode == NULL) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) return -1;  // No free file descriptors
    
    // Resolve path
    vnode_t* vnode = resolve_path(pathname);
    if (!vnode) {
        // File doesn't exist, create if O_CREAT is set
        if (flags & O_CREAT) {
            // Create file (not implemented for simplicity)
            return -1;
        }
        return -1;  // File not found
    }
    
    // Set up file table entry
    file_table[fd].vnode = vnode;
    file_table[fd].offset = 0;
    file_table[fd].flags = flags;
    file_table[fd].ref_count = 1;
    
    // Truncate file if O_TRUNC is set
    if ((flags & O_TRUNC) && vnode->inode->mode == FT_REGULAR) {
        vnode->inode->size = 0;
        // Free all blocks (not implemented for simplicity)
    }
    
    return fd;
}

// Close file
int vfs_close(int fd) {
    if (fd < 0 || fd >= 256 || file_table[fd].vnode == NULL) {
        return -1;
    }
    
    file_table[fd].ref_count--;
    if (file_table[fd].ref_count == 0) {
        file_table[fd].vnode = NULL;
    }
    
    return 0;
}

// Read from file
ssize_t vfs_read(int fd, void* buffer, size_t count) {
    if (fd < 0 || fd >= 256 || file_table[fd].vnode == NULL) {
        return -1;
    }
    
    file_t* file = &file_table[fd];
    
    // Check if file is opened for reading
    if (!(file->flags & O_RDONLY) && !(file->flags & O_RDWR)) {
        return -1;
    }
    
    if (!file->vnode->ops || !file->vnode->ops->read) {
        return -1;
    }
    
    return file->vnode->ops->read(file->vnode->private_data, buffer, count);
}

// Write to file
ssize_t vfs_write(int fd, const void* buffer, size_t count) {
    if (fd < 0 || fd >= 256 || file_table[fd].vnode == NULL) {
        return -1;
    }
    
    file_t* file = &file_table[fd];
    
    // Check if file is opened for writing
    if (!(file->flags & O_WRONLY) && !(file->flags & O_RDWR)) {
        return -1;
    }
    
    if (!file->vnode->ops || !file->vnode->ops->write) {
        return -1;
    }
    
    return file->vnode->ops->write(file->vnode->private_data, buffer, count);
}

// Seek in file
int vfs_seek(int fd, uint32_t offset, int whence) {
    if (fd < 0 || fd >= 256 || file_table[fd].vnode == NULL) {
        return -1;
    }
    
    file_t* file = &file_table[fd];
    
    if (!file->vnode->ops || !file->vnode->ops->seek) {
        return -1;
    }
    
    return file->vnode->ops->seek(file->vnode->private_data, offset, whence);
}

// I/O control
int vfs_ioctl(int fd, uint32_t request, void* arg) {
    if (fd < 0 || fd >= 256 || file_table[fd].vnode == NULL) {
        return -1;
    }
    
    file_t* file = &file_table[fd];
    
    if (!file->vnode->ops || !file->vnode->ops->ioctl) {
        return -1;
    }
    
    return file->vnode->ops->ioctl(file->vnode->private_data, request, arg);
}

// Create directory
int vfs_mkdir(const char* pathname) {
    // Resolve parent directory
    char parent_path[512];
    char dir_name[256];
    
    // Split path into parent and directory name
    const char* last_slash = strrchr(pathname, '/');
    if (!last_slash) return -1;
    
    size_t parent_len = last_slash - pathname;
    strncpy(parent_path, pathname, parent_len);
    parent_path[parent_len] = '\0';
    strcpy(dir_name, last_slash + 1);
    
    vnode_t* parent = resolve_path(parent_path);
    if (!parent || parent->inode->mode != FT_DIRECTORY) {
        return -1;
    }
    
    // Allocate new inode
    uint32_t new_inode = alloc_inode();
    if (new_inode == 0) return -1;
    
    // Initialize directory inode
    inode_t* dir_inode = &inode_table[new_inode - 1];
    dir_inode->mode = FT_DIRECTORY | PERM_READ | PERM_WRITE | PERM_EXEC;
    dir_inode->uid = 0;
    dir_inode->gid = 0;
    dir_inode->size = SOLIXFS_BLOCK_SIZE;  // One block for . and ..
    dir_inode->atime = timer_get_ticks();
    dir_inode->mtime = timer_get_ticks();
    dir_inode->ctime = timer_get_ticks();
    dir_inode->links = 2;  // . and parent
    dir_inode->blocks = 1;
    
    // Allocate block for directory
    uint32_t block = alloc_block();
    if (block == 0) {
        free_inode(new_inode);
        return -1;
    }
    
    dir_inode->direct[0] = block;
    
    // Create directory entries
    dir_entry_t entries[2];
    entries[0].inode = new_inode;
    strcpy(entries[0].name, ".");
    entries[1].inode = parent->inode_num;
    strcpy(entries[1].name, "..");
    
    // Write directory entries
    write_block(block, (uint8_t*)entries);
    
    // Add entry to parent directory
    // (Simplified - should find empty slot in parent)
    read_block(parent->inode->direct[0], disk_buffer);
    dir_entry_t* parent_entries = (dir_entry_t*)disk_buffer;
    uint32_t entry_count = SOLIXFS_BLOCK_SIZE / SOLIXFS_DIR_ENTRY_SIZE;
    
    for (uint32_t i = 0; i < entry_count; i++) {
        if (parent_entries[i].inode == 0) {
            parent_entries[i].inode = new_inode;
            strcpy(parent_entries[i].name, dir_name);
            write_block(parent->inode->direct[0], disk_buffer);
            break;
        }
    }
    
    return 0;
}

// Read directory
int vfs_readdir(const char* pathname, dir_entry_t* entries, int count) {
    vnode_t* vnode = resolve_path(pathname);
    if (!vnode || vnode->inode->mode != FT_DIRECTORY) {
        return -1;
    }
    
    file_t file;
    file.vnode = vnode;
    file.offset = 0;
    file.flags = O_RDONLY;
    
    ssize_t bytes_read = dir_read(vnode, entries, count * SOLIXFS_DIR_ENTRY_SIZE);
    return bytes_read / SOLIXFS_DIR_ENTRY_SIZE;
}

// Get file status
int vfs_stat(const char* pathname, inode_t* stat) {
    vnode_t* vnode = resolve_path(pathname);
    if (!vnode) return -1;
    
    *stat = *vnode->inode;
    return 0;
}
