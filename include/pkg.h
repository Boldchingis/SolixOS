#ifndef SOLIX_PKG_H
#define SOLIX_PKG_H

#include "types.h"

// Package manager constants
#define PKG_MAX_NAME 64
#define PKG_MAX_VERSION 32
#define PKG_MAX_DESC 256
#define PKG_MAX_DEPS 16
#define PKG_MAX_FILES 128
#define PKG_MAX_PATH 256

// Package states
#define PKG_STATE_AVAILABLE 0
#define PKG_STATE_INSTALLED 1
#define PKG_STATE_UPGRADABLE 2
#define PKG_STATE_BROKEN 3

// Package structure
typedef struct package {
    char name[PKG_MAX_NAME];
    char version[PKG_MAX_VERSION];
    char description[PKG_MAX_DESC];
    char dependencies[PKG_MAX_DEPS][PKG_MAX_NAME];
    int num_dependencies;
    char files[PKG_MAX_FILES][PKG_MAX_PATH];
    int num_files;
    uint32_t size;
    uint32_t installed_size;
    uint8_t state;
    uint32_t install_date;
} package_t;

// Package repository structure
typedef struct pkg_repo {
    char name[PKG_MAX_NAME];
    char url[PKG_MAX_PATH];
    char path[PKG_MAX_PATH];
    bool enabled;
    package_t* packages;
    int num_packages;
} pkg_repo_t;

// Package manager functions
void pkg_init(void);
int pkg_add_repo(const char* name, const char* url, const char* path);
int pkg_remove_repo(const char* name);
int pkg_update_repos(void);
int pkg_search(const char* pattern);
int pkg_info(const char* name);
int pkg_install(const char* name);
int pkg_remove(const char* name);
int pkg_upgrade(const char* name);
int pkg_list_installed(void);
int pkg_list_available(void);
int pkg_verify(const char* name);

// Package file operations
int pkg_download(const char* repo_name, const char* pkg_name, const char* dest_path);
int pkg_extract(const char* archive_path, const char* dest_path);
int pkg_verify_checksum(const char* file_path, const char* expected_checksum);
int pkg_install_files(const char* pkg_name);
int pkg_remove_files(const char* pkg_name);

// Package database operations
int pkg_db_load(void);
int pkg_db_save(void);
int pkg_db_add_package(const package_t* pkg);
int pkg_db_remove_package(const char* name);
int pkg_db_get_package(const char* name, package_t* pkg);
int pkg_db_update_package(const package_t* pkg);

// Package utility functions
int pkg_version_compare(const char* ver1, const char* ver2);
bool pkg_check_dependencies(const package_t* pkg);
int pkg_resolve_dependencies(const char* name, package_t** deps, int max_deps);
const char* pkg_state_to_string(uint8_t state);

#endif
