#include "pkg.h"
#include "vfs.h"
#include "net.h"
#include "screen.h"
#include "mm.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Package manager state
static pkg_repo_t repos[16];
static int num_repos = 0;
static package_t* packages = NULL;
static int num_packages = 0;
static bool db_loaded = false;

// Package database path
#define PKG_DB_PATH "/var/lib/pkg/packages.db"
#define PKG_CACHE_PATH "/var/cache/pkg"
#define PKG_CONFIG_PATH "/etc/pkg"

// Initialize package manager
void pkg_init(void) {
    memset(repos, 0, sizeof(repos));
    packages = NULL;
    num_packages = 0;
    db_loaded = false;
    
    // Create necessary directories
    vfs_mkdir("/var");
    vfs_mkdir("/var/lib");
    vfs_mkdir("/var/lib/pkg");
    vfs_mkdir("/var/cache");
    vfs_mkdir("/var/cache/pkg");
    vfs_mkdir("/etc");
    vfs_mkdir("/etc/pkg");
    
    // Load package database
    pkg_db_load();
    
    // Add default repository
    pkg_add_repo("main", "http://packages.solixos.org", "/var/cache/pkg/main");
    
    screen_print("Package manager initialized\n");
}

// Add repository
int pkg_add_repo(const char* name, const char* url, const char* path) {
    if (num_repos >= 16) {
        return -1;
    }
    
    // Check if repo already exists
    for (int i = 0; i < num_repos; i++) {
        if (strcmp(repos[i].name, name) == 0) {
            return -1;
        }
    }
    
    // Add new repo
    strcpy(repos[num_repos].name, name);
    strcpy(repos[num_repos].url, url);
    strcpy(repos[num_repos].path, path);
    repos[num_repos].enabled = true;
    repos[num_repos].packages = NULL;
    repos[num_repos].num_packages = 0;
    
    // Create repo directory
    vfs_mkdir(path);
    
    num_repos++;
    
    screen_print("Added repository: ");
    screen_print(name);
    screen_print("\n");
    
    return 0;
}

// Remove repository
int pkg_remove_repo(const char* name) {
    for (int i = 0; i < num_repos; i++) {
        if (strcmp(repos[i].name, name) == 0) {
            // Free packages
            if (repos[i].packages) {
                kfree(repos[i].packages);
            }
            
            // Remove repo
            for (int j = i; j < num_repos - 1; j++) {
                repos[j] = repos[j + 1];
            }
            num_repos--;
            
            screen_print("Removed repository: ");
            screen_print(name);
            screen_print("\n");
            
            return 0;
        }
    }
    
    return -1;
}

// Update repositories
int pkg_update_repos(void) {
    screen_print("Updating package repositories...\n");
    
    for (int i = 0; i < num_repos; i++) {
        if (!repos[i].enabled) continue;
        
        screen_print("Updating ");
        screen_print(repos[i].name);
        screen_print("...\n");
        
        // Download package list
        char package_list_url[PKG_MAX_PATH];
        sprintf(package_list_url, "%s/packages.txt", repos[i].url);
        
        char local_path[PKG_MAX_PATH];
        sprintf(local_path, "%s/packages.txt", repos[i].path);
        
        if (pkg_download_file(package_list_url, local_path) != 0) {
            screen_print("Failed to download package list for ");
            screen_print(repos[i].name);
            screen_print("\n");
            continue;
        }
        
        // Parse package list
        pkg_parse_package_list(&repos[i], local_path);
    }
    
    screen_print("Repository update completed\n");
    return 0;
}

// Parse package list file
int pkg_parse_package_list(pkg_repo_t* repo, const char* file_path) {
    int fd = vfs_open(file_path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    char buffer[4096];
    ssize_t bytes = vfs_read(fd, buffer, sizeof(buffer) - 1);
    vfs_close(fd);
    
    if (bytes <= 0) {
        return -1;
    }
    
    buffer[bytes] = '\0';
    
    // Count packages
    int package_count = 0;
    char* ptr = buffer;
    while (*ptr) {
        if (*ptr == '\n') package_count++;
        ptr++;
    }
    
    if (package_count == 0) {
        return 0;
    }
    
    // Allocate packages array
    if (repo->packages) {
        kfree(repo->packages);
    }
    repo->packages = kmalloc(package_count * sizeof(package_t));
    if (!repo->packages) {
        return -1;
    }
    
    // Parse packages
    ptr = buffer;
    int index = 0;
    char line[512];
    
    while (*ptr && index < package_count) {
        // Get next line
        char* line_start = ptr;
        while (*ptr && *ptr != '\n') ptr++;
        int line_len = ptr - line_start;
        
        if (line_len > 0 && line_len < sizeof(line)) {
            strncpy(line, line_start, line_len);
            line[line_len] = '\0';
            
            // Parse package line: name version size description
            package_t* pkg = &repo->packages[index];
            memset(pkg, 0, sizeof(package_t));
            
            char* token = strtok(line, " ");
            if (token) {
                strncpy(pkg->name, token, PKG_MAX_NAME - 1);
                
                token = strtok(NULL, " ");
                if (token) {
                    strncpy(pkg->version, token, PKG_MAX_VERSION - 1);
                    
                    token = strtok(NULL, " ");
                    if (token) {
                        pkg->size = atoi(token);
                        
                        token = strtok(NULL, "");
                        if (token) {
                            strncpy(pkg->description, token, PKG_MAX_DESC - 1);
                            pkg->state = PKG_STATE_AVAILABLE;
                            index++;
                        }
                    }
                }
            }
        }
        
        if (*ptr == '\n') ptr++;
    }
    
    repo->num_packages = index;
    
    screen_print("Parsed ");
    screen_print_dec(index);
    screen_print(" packages from ");
    screen_print(repo->name);
    screen_print("\n");
    
    return 0;
}

// Search packages
int pkg_search(const char* pattern) {
    screen_print("Searching for packages matching: ");
    screen_print(pattern);
    screen_print("\n\n");
    
    int found = 0;
    
    for (int i = 0; i < num_repos; i++) {
        if (!repos[i].enabled) continue;
        
        for (int j = 0; j < repos[i].num_packages; j++) {
            package_t* pkg = &repos[i].packages[j];
            
            // Check if pattern matches name or description
            if (strcasestr(pkg->name, pattern) || strcasestr(pkg->description, pattern)) {
                screen_print(pkg->name);
                screen_print(" ");
                screen_print(pkg->version);
                screen_print(" - ");
                screen_print(pkg->description);
                screen_print(" [");
                screen_print(repos[i].name);
                screen_print("]\n");
                found++;
            }
        }
    }
    
    if (found == 0) {
        screen_print("No packages found\n");
    } else {
        screen_print("\nFound ");
        screen_print_dec(found);
        screen_print(" packages\n");
    }
    
    return 0;
}

// Show package information
int pkg_info(const char* name) {
    // Find package
    package_t* pkg = NULL;
    pkg_repo_t* repo = NULL;
    
    for (int i = 0; i < num_repos; i++) {
        if (!repos[i].enabled) continue;
        
        for (int j = 0; j < repos[i].num_packages; j++) {
            if (strcmp(repos[i].packages[j].name, name) == 0) {
                pkg = &repos[i].packages[j];
                repo = &repos[i];
                break;
            }
        }
        if (pkg) break;
    }
    
    if (!pkg) {
        screen_print("Package not found: ");
        screen_print(name);
        screen_print("\n");
        return -1;
    }
    
    // Display package info
    screen_print("Package: ");
    screen_print(pkg->name);
    screen_print("\n");
    screen_print("Version: ");
    screen_print(pkg->version);
    screen_print("\n");
    screen_print("Repository: ");
    screen_print(repo->name);
    screen_print("\n");
    screen_print("Description: ");
    screen_print(pkg->description);
    screen_print("\n");
    screen_print("Size: ");
    screen_print_dec(pkg->size);
    screen_print(" bytes\n");
    screen_print("State: ");
    screen_print(pkg_state_to_string(pkg->state));
    screen_print("\n");
    
    if (pkg->num_dependencies > 0) {
        screen_print("Dependencies:\n");
        for (int i = 0; i < pkg->num_dependencies; i++) {
            screen_print("  ");
            screen_print(pkg->dependencies[i]);
            screen_print("\n");
        }
    }
    
    return 0;
}

// Install package
int pkg_install(const char* name) {
    screen_print("Installing ");
    screen_print(name);
    screen_print("...\n");
    
    // Find package
    package_t* pkg = NULL;
    pkg_repo_t* repo = NULL;
    
    for (int i = 0; i < num_repos; i++) {
        if (!repos[i].enabled) continue;
        
        for (int j = 0; j < repos[i].num_packages; j++) {
            if (strcmp(repos[i].packages[j].name, name) == 0) {
                pkg = &repos[i].packages[j];
                repo = &repos[i];
                break;
            }
        }
        if (pkg) break;
    }
    
    if (!pkg) {
        screen_print("Package not found: ");
        screen_print(name);
        screen_print("\n");
        return -1;
    }
    
    // Check if already installed
    if (pkg->state == PKG_STATE_INSTALLED) {
        screen_print("Package already installed\n");
        return 0;
    }
    
    // Check dependencies
    if (!pkg_check_dependencies(pkg)) {
        screen_print("Dependency check failed\n");
        return -1;
    }
    
    // Download package
    char package_url[PKG_MAX_PATH];
    sprintf(package_url, "%s/%s-%s.pkg", repo->url, pkg->name, pkg->version);
    
    char local_path[PKG_MAX_PATH];
    sprintf(local_path, "%s/%s-%s.pkg", repo->path, pkg->name, pkg->version);
    
    screen_print("Downloading package...\n");
    if (pkg_download_file(package_url, local_path) != 0) {
        screen_print("Failed to download package\n");
        return -1;
    }
    
    // Extract package
    char extract_path[PKG_MAX_PATH];
    sprintf(extract_path, "%s/%s-%s", repo->path, pkg->name, pkg->version);
    vfs_mkdir(extract_path);
    
    screen_print("Extracting package...\n");
    if (pkg_extract(local_path, extract_path) != 0) {
        screen_print("Failed to extract package\n");
        return -1;
    }
    
    // Install files
    screen_print("Installing files...\n");
    if (pkg_install_from_directory(extract_path) != 0) {
        screen_print("Failed to install files\n");
        return -1;
    }
    
    // Update package state
    pkg->state = PKG_STATE_INSTALLED;
    pkg->install_date = timer_get_ticks();
    
    // Save database
    pkg_db_save();
    
    screen_print("Package ");
    screen_print(name);
    screen_print(" installed successfully\n");
    
    return 0;
}

// Remove package
int pkg_remove(const char* name) {
    screen_print("Removing ");
    screen_print(name);
    screen_print("...\n");
    
    // Find installed package
    package_t* pkg = NULL;
    
    for (int i = 0; i < num_repos; i++) {
        for (int j = 0; j < repos[i].num_packages; j++) {
            if (strcmp(repos[i].packages[j].name, name) == 0 && 
                repos[i].packages[j].state == PKG_STATE_INSTALLED) {
                pkg = &repos[i].packages[j];
                break;
            }
        }
        if (pkg) break;
    }
    
    if (!pkg) {
        screen_print("Package not installed: ");
        screen_print(name);
        screen_print("\n");
        return -1;
    }
    
    // Remove files
    for (int i = 0; i < pkg->num_files; i++) {
        vfs_unlink(pkg->files[i]);
    }
    
    // Update package state
    pkg->state = PKG_STATE_AVAILABLE;
    
    // Save database
    pkg_db_save();
    
    screen_print("Package ");
    screen_print(name);
    screen_print(" removed successfully\n");
    
    return 0;
}

// List installed packages
int pkg_list_installed(void) {
    screen_print("Installed packages:\n\n");
    
    int count = 0;
    
    for (int i = 0; i < num_repos; i++) {
        for (int j = 0; j < repos[i].num_packages; j++) {
            package_t* pkg = &repos[i].packages[j];
            if (pkg->state == PKG_STATE_INSTALLED) {
                screen_print(pkg->name);
                screen_print(" ");
                screen_print(pkg->version);
                screen_print(" - ");
                screen_print(pkg->description);
                screen_print("\n");
                count++;
            }
        }
    }
    
    if (count == 0) {
        screen_print("No packages installed\n");
    } else {
        screen_print("\nTotal: ");
        screen_print_dec(count);
        screen_print(" packages\n");
    }
    
    return 0;
}

// List available packages
int pkg_list_available(void) {
    screen_print("Available packages:\n\n");
    
    int count = 0;
    
    for (int i = 0; i < num_repos; i++) {
        if (!repos[i].enabled) continue;
        
        screen_print("Repository: ");
        screen_print(repos[i].name);
        screen_print("\n");
        
        for (int j = 0; j < repos[i].num_packages; j++) {
            package_t* pkg = &repos[i].packages[j];
            if (pkg->state == PKG_STATE_AVAILABLE) {
                screen_print("  ");
                screen_print(pkg->name);
                screen_print(" ");
                screen_print(pkg->version);
                screen_print(" - ");
                screen_print(pkg->description);
                screen_print("\n");
                count++;
            }
        }
        screen_print("\n");
    }
    
    if (count == 0) {
        screen_print("No packages available\n");
    } else {
        screen_print("Total: ");
        screen_print_dec(count);
        screen_print(" packages\n");
    }
    
    return 0;
}

// Utility functions

// Case-insensitive string search
char* strcasestr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && tolower(*h) == tolower(*n)) {
            h++;
            n++;
        }
        
        if (!*n) return (char*)haystack;
        haystack++;
    }
    
    return NULL;
}

// Convert package state to string
const char* pkg_state_to_string(uint8_t state) {
    switch (state) {
        case PKG_STATE_AVAILABLE:
            return "Available";
        case PKG_STATE_INSTALLED:
            return "Installed";
        case PKG_STATE_UPGRADABLE:
            return "Upgradable";
        case PKG_STATE_BROKEN:
            return "Broken";
        default:
            return "Unknown";
    }
}

// Check package dependencies
bool pkg_check_dependencies(const package_t* pkg) {
    for (int i = 0; i < pkg->num_dependencies; i++) {
        // Check if dependency is installed
        bool found = false;
        for (int j = 0; j < num_repos; j++) {
            for (int k = 0; k < repos[j].num_packages; k++) {
                if (strcmp(repos[j].packages[k].name, pkg->dependencies[i]) == 0 &&
                    repos[j].packages[k].state == PKG_STATE_INSTALLED) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        
        if (!found) {
            screen_print("Missing dependency: ");
            screen_print(pkg->dependencies[i]);
            screen_print("\n");
            return false;
        }
    }
    
    return true;
}

// Download file from URL
int pkg_download_file(const char* url, const char* dest_path) {
    // Parse URL
    char host[256];
    char path[512];
    
    if (sscanf(url, "http://%255[^/]%511[^\n]", host, path) != 2) {
        return -1;
    }
    
    // Create TCP connection
    socket_t sock;
    sock.type = SOCK_STREAM;
    sock.protocol = IPPROTO_TCP;
    sock.local_ip = net_get_device("eth0")->ip_addr;
    sock.local_port = 12345;
    sock.remote_ip = ip_aton(host);
    sock.remote_port = 80;
    
    if (tcp_connect(&sock, sock.remote_ip, 80) != 0) {
        return -1;
    }
    
    // Send HTTP request
    char request[1024];
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    
    if (tcp_send(&sock, request, strlen(request)) != 0) {
        return -1;
    }
    
    // Receive response
    char buffer[4096];
    int total_received = 0;
    
    // Skip headers
    while (1) {
        int bytes = tcp_receive(&sock, buffer, sizeof(buffer));
        if (bytes <= 0) return -1;
        
        char* body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4;
            int body_len = bytes - (body - buffer);
            
            // Write to file
            int fd = vfs_open(dest_path, O_CREAT | O_WRONLY);
            if (fd < 0) return -1;
            
            vfs_write(fd, body, body_len);
            vfs_close(fd);
            
            total_received += body_len;
            break;
        }
    }
    
    // Receive remaining data
    while (1) {
        int bytes = tcp_receive(&sock, buffer, sizeof(buffer));
        if (bytes <= 0) break;
        
        int fd = vfs_open(dest_path, O_WRONLY | O_APPEND);
        if (fd < 0) return -1;
        
        vfs_write(fd, buffer, bytes);
        vfs_close(fd);
        
        total_received += bytes;
    }
    
    return total_received;
}

// Extract package archive (simplified)
int pkg_extract(const char* archive_path, const char* dest_path) {
    // This is a simplified implementation
    // In a real implementation, you'd use a proper archive format like tar.gz
    
    screen_print("Extracting ");
    screen_print(archive_path);
    screen_print(" to ");
    screen_print(dest_path);
    screen_print("\n");
    
    // For now, just create a dummy file structure
    char dummy_file[PKG_MAX_PATH];
    sprintf(dummy_file, "%s/dummy.txt", dest_path);
    
    int fd = vfs_open(dummy_file, O_CREAT | O_WRONLY);
    if (fd >= 0) {
        vfs_write(fd, "Package file", 12);
        vfs_close(fd);
    }
    
    return 0;
}

// Install files from directory
int pkg_install_from_directory(const char* dir_path) {
    // Simplified installation - just copy files
    // In a real implementation, you'd read a manifest file
    
    screen_print("Installing from ");
    screen_print(dir_path);
    screen_print("\n");
    
    return 0;
}

// Database operations (simplified)
int pkg_db_load(void) {
    if (db_loaded) return 0;
    
    int fd = vfs_open(PKG_DB_PATH, O_RDONLY);
    if (fd < 0) {
        screen_print("Creating new package database\n");
        return 0;  // Database doesn't exist yet
    }
    
    // Load database (simplified)
    vfs_close(fd);
    db_loaded = true;
    
    return 0;
}

int pkg_db_save(void) {
    int fd = vfs_open(PKG_DB_PATH, O_CREAT | O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    
    // Save database (simplified)
    vfs_close(fd);
    
    return 0;
}
