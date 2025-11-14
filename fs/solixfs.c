#include "vfs.h"
#include "mm.h"
#include "screen.h"
#include "../include/disk.h"

// SolixFS constants
#define SOLIXFS_MAGIC 0x534F4C58  // "SOLX"
#define SOLIXFS_VERSION 1
#define SOLIXFS_BLOCK_SIZE 4096
#define SOLIXFS_INODE_SIZE sizeof(inode_t)
#define SOLIXFS_DIR_ENTRY_SIZE sizeof(dir_entry_t)

// Filesystem state
static superblock_t sb;
static uint8_t* block_bitmap = NULL;
static uint8_t* inode_bitmap = NULL;
static inode_t* inode_table = NULL;
static uint8_t* disk_buffer = NULL;

// Current file table
static file_t file_table[256];
static vnode_t* root_vnode = NULL;

// Initialize SolixFS
void solixfs_init(void) {
    // Read superblock from disk
    disk_read(0, (uint8_t*)&sb, sizeof(superblock_t));
    
    // Verify filesystem
    if (sb.magic != SOLIXFS_MAGIC) {
        screen_print("Invalid SolixFS filesystem\n");
        return;
    }
    
    // Allocate memory for filesystem structures
    uint32_t bitmap_size = (sb.total_blocks + 7) / 8;
    block_bitmap = kmalloc(bitmap_size);
    inode_bitmap = kmalloc((sb.inode_count + 7) / 8);
    inode_table = kmalloc(sb.inode_count * SOLIXFS_INODE_SIZE);
    disk_buffer = kmalloc(SOLIXFS_BLOCK_SIZE);
    
    // Read bitmaps and inode table
    disk_read(sb.inode_table, (uint8_t*)inode_table, sb.inode_count * SOLIXFS_INODE_SIZE);
    disk_read(sb.inode_table + sb.inode_count * SOLIXFS_INODE_SIZE / SOLIXFS_BLOCK_SIZE, 
              block_bitmap, bitmap_size);
    disk_read(sb.inode_table + sb.inode_count * SOLIXFS_INODE_SIZE / SOLIXFS_BLOCK_SIZE + 
              bitmap_size / SOLIXFS_BLOCK_SIZE, inode_bitmap, (sb.inode_count + 7) / 8);
    
    // Initialize root vnode
    root_vnode = kmalloc(sizeof(vnode_t));
    root_vnode->inode_num = 1;  // Root inode is always 1
    root_vnode->inode = &inode_table[0];
    root_vnode->parent = NULL;
    root_vnode->children = NULL;
    root_vnode->next = NULL;
    root_vnode->ops = &dir_ops;
    root_vnode->private_data = NULL;
    
    // Initialize file table
    for (int i = 0; i < 256; i++) {
        file_table[i].vnode = NULL;
        file_table[i].offset = 0;
        file_table[i].flags = 0;
        file_table[i].ref_count = 0;
    }
    
    screen_print("SolixFS initialized successfully\n");
}

// Allocate a block
static uint32_t alloc_block(void) {
    for (uint32_t i = 0; i < sb.total_blocks; i++) {
        uint32_t byte = i / 8;
        uint32_t bit = i % 8;
        
        if (!(block_bitmap[byte] & (1 << bit))) {
            block_bitmap[byte] |= (1 << bit);
            sb.free_blocks--;
            return i;
        }
    }
    return 0;  // No free blocks
}

// Free a block
static void free_block(uint32_t block) {
    uint32_t byte = block / 8;
    uint32_t bit = block % 8;
    
    if (block_bitmap[byte] & (1 << bit)) {
        block_bitmap[byte] &= ~(1 << bit);
        sb.free_blocks++;
    }
}

// Allocate an inode
static uint32_t alloc_inode(void) {
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        uint32_t byte = i / 8;
        uint32_t bit = i % 8;
        
        if (!(inode_bitmap[byte] & (1 << bit))) {
            inode_bitmap[byte] |= (1 << bit);
            sb.free_inodes--;
            return i + 1;  // Inodes are 1-based
        }
    }
    return 0;  // No free inodes
}

// Free an inode
static void free_inode(uint32_t inode) {
    if (inode == 0) return;
    
    uint32_t index = inode - 1;
    uint32_t byte = index / 8;
    uint32_t bit = index % 8;
    
    if (inode_bitmap[byte] & (1 << bit)) {
        inode_bitmap[byte] &= ~(1 << bit);
        sb.free_inodes++;
    }
}

// Read block from disk
static void read_block(uint32_t block, uint8_t* buffer) {
    disk_read(sb.data_blocks + block, buffer, SOLIXFS_BLOCK_SIZE);
}

// Write block to disk
static void write_block(uint32_t block, const uint8_t* buffer) {
    disk_write(sb.data_blocks + block, buffer, SOLIXFS_BLOCK_SIZE);
}

// Find file in directory
static uint32_t find_in_dir(uint32_t dir_inode, const char* name) {
    inode_t* dir = &inode_table[dir_inode - 1];
    
    if (dir->mode != FT_DIRECTORY) {
        return 0;
    }
    
    // Read directory entries
    for (uint32_t i = 0; i < dir->blocks && i < 12; i++) {
        read_block(dir->direct[i], disk_buffer);
        
        dir_entry_t* entries = (dir_entry_t*)disk_buffer;
        uint32_t entry_count = SOLIXFS_BLOCK_SIZE / SOLIXFS_DIR_ENTRY_SIZE;
        
        for (uint32_t j = 0; j < entry_count; j++) {
            if (entries[j].inode != 0 && strcmp(entries[j].name, name) == 0) {
                return entries[j].inode;
            }
        }
    }
    
    return 0;  // Not found
}

// Directory file operations
static ssize_t dir_read(void* private_data, void* buffer, size_t count) {
    vnode_t* vnode = (vnode_t*)private_data;
    inode_t* dir = vnode->inode;
    
    if (dir->mode != FT_DIRECTORY) {
        return -1;
    }
    
    // For simplicity, return directory entries as fixed-size records
    dir_entry_t* entries = (dir_entry_t*)buffer;
    uint32_t entry_count = count / SOLIXFS_DIR_ENTRY_SIZE;
    uint32_t entries_read = 0;
    
    // Calculate which block we're reading from
    uint32_t block_offset = vnode->offset / SOLIXFS_BLOCK_SIZE;
    uint32_t entry_offset = (vnode->offset % SOLIXFS_BLOCK_SIZE) / SOLIXFS_DIR_ENTRY_SIZE;
    
    if (block_offset >= dir->blocks || block_offset >= 12) {
        return 0;  // End of directory
    }
    
    read_block(dir->direct[block_offset], disk_buffer);
    dir_entry_t* dir_entries = (dir_entry_t*)disk_buffer;
    uint32_t entries_per_block = SOLIXFS_BLOCK_SIZE / SOLIXFS_DIR_ENTRY_SIZE;
    
    for (uint32_t i = entry_offset; i < entries_per_block && entries_read < entry_count; i++) {
        if (dir_entries[i].inode != 0) {
            entries[entries_read] = dir_entries[i];
            entries_read++;
            vnode->offset += SOLIXFS_DIR_ENTRY_SIZE;
        }
    }
    
    return entries_read * SOLIXFS_DIR_ENTRY_SIZE;
}

static ssize_t dir_write(void* private_data, const void* buffer, size_t count) {
    return -1;  // Directories are read-only
}

static int dir_seek(void* private_data, uint32_t offset, int whence) {
    vnode_t* vnode = (vnode_t*)private_data;
    
    switch (whence) {
        case SEEK_SET:
            vnode->offset = offset;
            break;
        case SEEK_CUR:
            vnode->offset += offset;
            break;
        case SEEK_END:
            vnode->offset = vnode->inode->size;
            break;
        default:
            return -1;
    }
    
    if (vnode->offset > vnode->inode->size) {
        vnode->offset = vnode->inode->size;
    }
    
    return 0;
}

static int dir_close(void* private_data) {
    // Nothing to do for directories
    return 0;
}

// Directory operations structure
file_ops_t dir_ops = {
    .read = dir_read,
    .write = dir_write,
    .seek = dir_seek,
    .close = dir_close
};

// Regular file operations
static ssize_t file_read(void* private_data, void* buffer, size_t count) {
    vnode_t* vnode = (vnode_t*)private_data;
    inode_t* file = vnode->inode;
    
    if (vnode->offset >= file->size) {
        return 0;  // EOF
    }
    
    uint32_t bytes_to_read = count;
    if (vnode->offset + bytes_to_read > file->size) {
        bytes_to_read = file->size - vnode->offset;
    }
    
    uint8_t* buf = (uint8_t*)buffer;
    uint32_t bytes_read = 0;
    
    while (bytes_read < bytes_to_read) {
        uint32_t block_offset = vnode->offset / SOLIXFS_BLOCK_SIZE;
        uint32_t offset_in_block = vnode->offset % SOLIXFS_BLOCK_SIZE;
        uint32_t bytes_in_block = SOLIXFS_BLOCK_SIZE - offset_in_block;
        
        if (bytes_in_block > bytes_to_read - bytes_read) {
            bytes_in_block = bytes_to_read - bytes_read;
        }
        
        // Read block
        if (block_offset < 12) {
            read_block(file->direct[block_offset], disk_buffer);
        } else {
            // Handle indirect blocks (not implemented for simplicity)
            break;
        }
        
        // Copy data
        memcpy(buf + bytes_read, disk_buffer + offset_in_block, bytes_in_block);
        
        bytes_read += bytes_in_block;
        vnode->offset += bytes_in_block;
    }
    
    return bytes_read;
}

static ssize_t file_write(void* private_data, const void* buffer, size_t count) {
    vnode_t* vnode = (vnode_t*)private_data;
    inode_t* file = vnode->inode;
    
    uint8_t* buf = (uint8_t*)buffer;
    uint32_t bytes_written = 0;
    
    while (bytes_written < count) {
        uint32_t block_offset = vnode->offset / SOLIXFS_BLOCK_SIZE;
        uint32_t offset_in_block = vnode->offset % SOLIXFS_BLOCK_SIZE;
        uint32_t bytes_in_block = SOLIXFS_BLOCK_SIZE - offset_in_block;
        
        if (bytes_in_block > count - bytes_written) {
            bytes_in_block = count - bytes_written;
        }
        
        // Allocate block if needed
        if (block_offset >= 12) {
            // Handle indirect blocks (not implemented for simplicity)
            break;
        }
        
        if (file->direct[block_offset] == 0) {
            uint32_t new_block = alloc_block();
            if (new_block == 0) {
                break;  // No space left
            }
            file->direct[block_offset] = new_block;
            file->blocks++;
        }
        
        // Read block (if not writing entire block)
        if (offset_in_block > 0 || bytes_in_block < SOLIXFS_BLOCK_SIZE) {
            read_block(file->direct[block_offset], disk_buffer);
        }
        
        // Write data
        memcpy(disk_buffer + offset_in_block, buf + bytes_written, bytes_in_block);
        write_block(file->direct[block_offset], disk_buffer);
        
        bytes_written += bytes_in_block;
        vnode->offset += bytes_in_block;
        
        // Update file size
        if (vnode->offset > file->size) {
            file->size = vnode->offset;
        }
    }
    
    return bytes_written;
}

static int file_seek(void* private_data, uint32_t offset, int whence) {
    vnode_t* vnode = (vnode_t*)private_data;
    
    switch (whence) {
        case SEEK_SET:
            vnode->offset = offset;
            break;
        case SEEK_CUR:
            vnode->offset += offset;
            break;
        case SEEK_END:
            vnode->offset = vnode->inode->size;
            break;
        default:
            return -1;
    }
    
    if (vnode->offset > vnode->inode->size) {
        vnode->offset = vnode->inode->size;
    }
    
    return 0;
}

static int file_close(void* private_data) {
    // Nothing to do for regular files
    return 0;
}

// File operations structure
file_ops_t file_ops = {
    .read = file_read,
    .write = file_write,
    .seek = file_seek,
    .close = file_close
};
