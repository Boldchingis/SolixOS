#ifndef SOLIX_LINUX_VFS_H
#define SOLIX_LINUX_VFS_H

#include "types.h"
#include "vfs.h"

/**
 * Linux-Inspired Virtual File System (VFS) Layer for SolixOS
 * Comprehensive filesystem abstraction with proper VFS operations
 * Based on Linux VFS design principles
 */

// Linux-style file types
#define S_IFMT      00170000  // File type mask
#define S_IFSOCK    0140000   // Socket
#define S_IFLNK     0120000   // Symbolic link
#define S_IFREG     0100000   // Regular file
#define S_IFBLK     0060000   // Block device
#define S_IFDIR     0040000   // Directory
#define S_IFCHR     0020000   // Character device
#define S_IFIFO     0010000   // FIFO

// Linux-style file permissions
#define S_ISUID     0004000   // Set user ID
#define S_ISGID     0002000   // Set group ID
#define S_ISVTX     0001000   // Sticky bit
#define S_IRWXU     0000700   // Owner permissions
#define S_IRUSR     0000400   // Owner read
#define S_IWUSR     0000200   // Owner write
#define S_IXUSR     0000100   // Owner execute
#define S_IRWXG     0000070   // Group permissions
#define S_IRGRP     0000040   // Group read
#define S_IWGRP     0000020   // Group write
#define S_IXGRP     0000010   // Group execute
#define S_IRWXO     0000007   // Others permissions
#define S_IROTH     0000004   // Others read
#define S_IWOTH     0000002   // Others write
#define S_IXOTH     0000001   // Others execute

// File type test macros
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

// Open flags (Linux-style)
#define O_ACCMODE   00000003
#define O_RDONLY    00000000
#define O_WRONLY    00000001
#define O_RDWR      00000002
#define O_CREAT     00000100
#define O_EXCL      00000200
#define O_NOCTTY    00000400
#define O_TRUNC     00001000
#define O_APPEND    00002000
#define O_NONBLOCK  00004000
#define O_DSYNC     00010000
#define O_DIRECT    00040000
#define O_LARGEFILE 00100000
#define O_DIRECTORY 00200000
#define O_NOFOLLOW  00400000
#define O_NOATIME   01000000
#define O_CLOEXEC   02000000

// Seek constants
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_DATA   3
#define SEEK_HOLE   4

// Stat structure (Linux-style)
struct stat {
    uint32_t st_dev;         // Device ID
    uint32_t st_ino;         // Inode number
    uint32_t st_mode;        // File mode and type
    uint32_t st_nlink;       // Number of hard links
    uint32_t st_uid;         // User ID of owner
    uint32_t st_gid;         // Group ID of owner
    uint32_t st_rdev;        // Device ID (if special file)
    uint32_t st_size;        // Total size in bytes
    uint32_t st_blksize;     // Block size for filesystem I/O
    uint32_t st_blocks;      // Number of allocated blocks
    uint32_t st_atime;       // Last access time
    uint32_t st_mtime;       // Last modification time
    uint32_t st_ctime;       // Last status change time
};

// Extended stat structure
struct stat64 {
    uint64_t st_size;        // Total size in bytes (64-bit)
    uint32_t st_blksize;     // Block size for filesystem I/O
    uint64_t st_blocks;      // Number of allocated blocks (64-bit)
    uint32_t st_dev;         // Device ID
    uint32_t st_ino;         // Inode number
    uint32_t st_mode;        // File mode and type
    uint32_t st_nlink;       // Number of hard links
    uint32_t st_uid;         // User ID of owner
    uint32_t st_gid;         // Group ID of owner
    uint32_t st_rdev;        // Device ID (if special file)
    uint32_t st_atime;       // Last access time
    uint32_t st_mtime;       // Last modification time
    uint32_t st_ctime;       // Last status change time
};

// Filesystem types
#define FS_MAGIC_EXT2    0xEF53
#define FS_MAGIC_EXT3    0xEF53
#define FS_MAGIC_EXT4    0xEF53
#define FS_MAGIC_MINIX    0x137F
#define FS_MAGIC_MINIX2   0x2468
#define FS_MAGIC_MINIX3   0x2478
#define FS_MAGIC_REISER   0x52654973
#define FS_MAGIC_XFS      0x58465342
#define FS_MAGIC_BTRFS    0x31263453
#define FS_MAGIC_F2FS     0xF2F52010
#define FS_MAGIC_NFS      0x6969
#define FS_MAGIC_PROC     0x9FA0
#define FS_MAGIC_SYSFS    0x62656572
#define FS_MAGIC_TMPFS    0x01021994
#define FS_MAGIC_DEVPTS   0x1CD1
#define FS_MAGIC_SOCKFS   0x534F434B
#define FS_MAGIC_PIPEFS   0x50495045
#define FS_MAGIC_RAMFS    0x858458F6
#define FS_MAGIC_ROMFS    0x7275
#define FS_MAGIC_SQUASHFS 0x73717368

// Mount flags
#define MS_RDONLY        1       // Mount read-only
#define MS_NOSUID        2       // Ignore suid and sgid bits
#define MS_NODEV         4       // Disallow access to device special files
#define MS_NOEXEC        8       // Disallow program execution
#define MS_SYNCHRONOUS   16      // Writes are synced at once
#define MS_REMOUNT       32      // Alter flags of a mounted FS
#define MS_MANDLOCK      64      // Allow mandatory locks on an FS
#define MS_DIRSYNC       128     // Directory modifications are synchronous
#define MS_NOATIME       1024    // Do not update access times
#define MS_NODIRATIME    2048    // Do not update directory access times
#define MS_BIND          4096    // Bind directory at different place
#define MS_MOVE          8192    // Atomically move subtree
#define MS_REC           16384   // Recursive
#define MS_SILENT        32768   // Be quiet
#define MS_POSIXACL      65536   // POSIX ACLs
#define MS_UNBINDABLE    131072  // Change to unbindable
#define MS_PRIVATE       262144  // Change to private
#define MS_SLAVE         524288  // Change to slave
#define MS_SHARED        1048576 // Change to shared
#define MS_RELATIME      2097152 // Update atime relative to mtime/ctime
#define MS_KERNMOUNT     4194304 // This is a kern_mount call
#define MS_I_VERSION     8388608 // Update inode version
#define MS_STRICTATIME   16777216 // Always perform atime updates

// Filesystem information structure
struct statfs {
    uint32_t f_type;        // Type of filesystem
    uint32_t f_bsize;       // Optimal transfer block size
    uint32_t f_blocks;      // Total data blocks in filesystem
    uint32_t f_bfree;       // Free blocks in filesystem
    uint32_t f_bavail;      // Free blocks available to non-superuser
    uint32_t f_files;       // Total file nodes in filesystem
    uint32_t f_ffree;       // Free file nodes in filesystem
    uint32_t f_fsid;        // Filesystem ID
    uint32_t f_namelen;     // Maximum length of filenames
    uint32_t f_frsize;      // Fragment size
    uint32_t f_flags;       // Mount flags
    uint32_t f_spare[4];    // Spare for future
};

// VFS inode structure (Linux-style)
struct inode {
    uint32_t i_mode;        // File mode
    uint32_t i_uid;         // Owner UID
    uint32_t i_gid;         // Group GID
    uint32_t i_ino;         // Inode number
    uint32_t i_size;        // Size in bytes
    uint32_t i_blocks;      // Number of blocks
    uint32_t i_atime;       // Access time
    uint32_t i_mtime;       // Modification time
    uint32_t i_ctime;       // Creation time
    uint32_t i_nlink;       // Number of links
    uint32_t i_generation;  // File version (for NFS)
    uint32_t i_blksize;     // Block size
    uint32_t i_rdev;        // Device number
    uint32_t i_flags;       // File flags
    
    // Inode operations
    struct inode_operations *i_op;
    struct file_operations *i_fop;
    
    // Superblock pointer
    struct super_block *i_sb;
    
    // Filesystem-specific data
    void *i_private;
    
    // Locking
    spinlock_t i_lock;
    
    // Reference count
    uint32_t i_count;
    
    // List linkage
    struct list_head i_hash;
    struct list_head i_list;
    struct list_head i_sb_list;
    struct list_head i_dentry;
};

// Inode operations structure
struct inode_operations {
    int (*create) (struct inode *, struct dentry *, umode_t, bool);
    struct dentry * (*lookup) (struct inode *, struct dentry *, unsigned int flags);
    int (*link) (struct dentry *, struct inode *, struct dentry *);
    int (*unlink) (struct inode *, struct dentry *);
    int (*symlink) (struct inode *, struct dentry *, const char *);
    int (*mkdir) (struct inode *, struct dentry *, umode_t);
    int (*rmdir) (struct inode *, struct dentry *);
    int (*mknod) (struct inode *, struct dentry *, umode_t, dev_t);
    int (*rename) (struct inode *, struct dentry *, struct inode *, struct dentry *, unsigned int);
    int (*readlink) (struct dentry *, char __user *, int);
    const char * (*get_link) (struct dentry *, struct inode *, struct delayed_call *);
    int (*permission) (struct inode *, int);
    int (*get_acl) (struct inode *, int);
    int (*setattr) (struct dentry *, struct iattr *);
    int (*getattr) (const struct path *, struct kstat *, u32, unsigned int);
    ssize_t (*listxattr) (struct dentry *, char *, size_t);
    int (*fiemap) (struct inode *, struct fiemap_extent_info *, u64 start, u64 len);
};

// File operations structure
struct file_operations {
    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
    ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
    ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
    int (*iterate) (struct file *, struct dir_context *);
    int (*iterate_shared) (struct file *, struct dir_context *);
    __poll_t (*poll) (struct file *, struct poll_table_struct *);
    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
    long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
    int (*mmap) (struct file *, struct vm_area_struct *);
    int (*open) (struct inode *, struct file *);
    int (*flush) (struct file *, fl_owner_t id);
    int (*release) (struct inode *, struct file *);
    int (*fsync) (struct file *, loff_t, loff_t, int);
    int (*fasync) (int, struct file *, int);
    int (*lock) (struct file *, int, struct file_lock *);
    ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
    unsigned long (*get_unmapped_area) (struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
    int (*check_flags) (int);
    int (*flock) (struct file *, int, struct file_lock *);
    ssize_t (*splice_write) (struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
    ssize_t (*splice_read) (struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
    int (*setlease) (struct file *, long, struct file_lock **, void **);
    long (*fallocate) (struct file *file, int mode, loff_t offset, loff_t len);
    void (*show_fdinfo) (struct seq_file *m, struct file *f);
};

// Dentry structure (directory entry)
struct dentry {
    uint32_t d_name_len;    // Name length
    char d_name[256];       // Name
    struct inode *d_inode;  // Associated inode
    struct dentry *d_parent; // Parent directory
    struct list_head d_subdirs; // List of subdirectories
    struct list_head d_child; // List of child entries
    struct list_head d_hash; // Hash list
    struct super_block *d_sb; // Superblock
    unsigned int d_flags;   // Dentry flags
    spinlock_t d_lock;      // Dentry lock
    uint32_t d_count;       // Reference count
};

// Superblock structure
struct super_block {
    uint32_t s_dev;         // Device identifier
    unsigned long s_blocksize;  // Block size
    unsigned long s_old_blocksize; // Old block size
    unsigned long s_blocksize_bits; // Block size bits
    unsigned char s_dirt;      // Dirty flag
    unsigned long long s_maxbytes; // Max file size
    struct file_system_type *s_type; // Filesystem type
    struct super_operations *s_op;   // Superblock operations
    struct dentry *s_root;     // Root dentry
    struct list_head s_inodes;  // List of inodes
    struct list_head s_dirty;  // List of dirty inodes
    struct list_head s_io;     // List of inodes being written
    struct list_head s_more_io; // List of more inodes being written
    struct hlist_head s_anon;  // Anonymous dentries
    struct list_head s_mounts; // List of mounts
    struct block_device *s_bdev; // Block device
    char s_id[32];           // Textual name
    void *s_fs_info;         // Filesystem private info
    uint32_t s_flags;        // Mount flags
    uint32_t s_magic;        // Filesystem magic number
    uint32_t s_time_gran;    // Timestamp granularity
    spinlock_t s_inode_lock; // Inode list lock
    spinlock_t s_files_lock; // File list lock
};

// Superblock operations
struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
    void (*destroy_inode)(struct inode *);
    void (*dirty_inode) (struct inode *, int flags);
    int (*write_inode) (struct inode *, struct writeback_control *);
    int (*drop_inode) (struct inode *);
    void (*evict_inode) (struct inode *);
    void (*put_super) (struct super_block *);
    int (*sync_fs)(struct super_block *sb, int wait);
    int (*freeze_fs) (struct super_block *);
    int (*unfreeze_fs) (struct super_block *);
    int (*statfs) (struct dentry *, struct statfs *);
    int (*remount_fs) (struct super_block *, int *, char *);
    void (*umount_begin) (struct super_block *);
    int (*show_options)(struct seq_file *, struct dentry *);
    int (*show_devname)(struct seq_file *, struct dentry *);
    int (*show_path)(struct seq_file *, struct dentry *);
    int (*show_stats)(struct seq_file *, struct dentry *);
};

// File system type
struct file_system_type {
    const char *name;
    int fs_flags;
    struct dentry *(*mount) (struct file_system_type *, int, const char *, void *);
    void (*kill_sb) (struct super_block *);
    struct module *owner;
    struct file_system_type * next;
    struct hlist_head fs_supers;
};

// Mount structure
struct vfsmount {
    struct dentry *mnt_root;    // Root of the mounted tree
    struct super_block *mnt_sb; // Pointer to superblock
    struct vfsmount *mnt_parent; // Parent mount
    struct list_head mnt_mounts; // List of children
    struct list_head mnt_child;  // List of our children
    struct list_head mnt_hash;   // Mount hash list
    int mnt_flags;              // Mount flags
    char *mnt_devname;           // Name of device
};

// Path structure
struct path {
    struct vfsmount *mnt;
    struct dentry *dentry;
};

// File structure
struct file {
    struct path f_path;
    struct inode *f_inode;
    const struct file_operations *f_op;
    loff_t f_pos;
    unsigned int f_flags;
    unsigned int f_mode;
    loff_t f_version;
    void *private_data;
    struct list_head f_list;
    spinlock_t f_lock;
    unsigned int f_count;
};

// VFS functions
int vfs_init(void);
int vfs_mount(const char *dev_name, const char *dir_name, const char *type, unsigned long flags, void *data);
int vfs_umount(const char *target, int flags);

// File operations
struct file *filp_open(const char *filename, int flags, umode_t mode);
int filp_close(struct file *filp, fl_owner_t id);
ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos);
loff_t vfs_llseek(struct file *file, loff_t offset, int whence);
int vfs_stat(const char *name, struct stat *stat);
int vfs_fstat(int fd, struct stat *stat);

// Directory operations
int vfs_mkdir(const char *pathname, umode_t mode);
int vfs_rmdir(const char *pathname);
int vfs_mknod(const char *pathname, umode_t mode, dev_t dev);
int vfs_unlink(const char *pathname);
int vfs_symlink(const char *oldname, const char *newname);
int vfs_link(const char *oldname, const char *newname);
int vfs_rename(const char *oldname, const char *newname);

// Inode operations
struct inode *iget(struct super_block *sb, unsigned long ino);
void iput(struct inode *inode);
int inode_permission(struct inode *inode, int mask);
int generic_permission(struct inode *inode, int mask);

// Dentry operations
struct dentry *d_lookup(struct dentry *parent, struct qstr *name);
int d_add(struct dentry *dentry, struct inode *inode);
void d_invalidate(struct dentry *dentry);

// File system registration
int register_filesystem(struct file_system_type *fs);
int unregister_filesystem(struct file_system_type *fs);
struct file_system_type *get_fs_type(const char *name);

// Path resolution
int kern_path(const char *name, unsigned int flags, struct path *path);
int path_lookup(const char *name, unsigned int flags, struct nameidata *nd);
int user_path_at_empty(int dfd, const char __user *name, unsigned flags,
                      struct path *path, int *empty);

// Helper macros
#define INIT_LIST_HEAD(ptr) do { (ptr)->next = (ptr); (ptr)->prev = (ptr); } while (0)
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member), \
        n = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = list_entry(n->member.next, typeof(*pos), member))

#endif
