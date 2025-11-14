#include "shell.h"
#include "screen.h"
#include "keyboard.h"
#include "vfs.h"
#include "kernel.h"
#include "mm.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

// Shell state
static char current_dir[MAX_PATH] = "/";
static char command_line[MAX_CMD_LEN];
static command_t commands[64];
static int num_commands = 0;

// Start shell
void shell_start(void) {
    screen_print("SolixOS Shell v1.0\n");
    screen_print("Type 'help' for available commands\n\n");
    
    // Register built-in commands
    shell_register_command("help", cmd_help, "Show this help message");
    shell_register_command("clear", cmd_clear, "Clear the screen");
    shell_register_command("ls", cmd_ls, "List directory contents");
    shell_register_command("cd", cmd_cd, "Change directory");
    shell_register_command("pwd", cmd_pwd, "Print working directory");
    shell_register_command("cat", cmd_cat, "Display file contents");
    shell_register_command("echo", cmd_echo, "Echo arguments");
    shell_register_command("mkdir", cmd_mkdir, "Create directory");
    shell_register_command("touch", cmd_touch, "Create empty file");
    shell_register_command("rm", cmd_rm, "Remove file or directory");
    shell_register_command("ps", cmd_ps, "List processes");
    shell_register_command("kill", cmd_kill, "Terminate process");
    shell_register_command("reboot", cmd_reboot, "Reboot system");
    shell_register_command("halt", cmd_halt, "Halt system");
    shell_register_command("meminfo", cmd_meminfo, "Show memory information");
    shell_register_command("mount", cmd_mount, "Mount filesystem");
    shell_register_command("umount", cmd_umount, "Unmount filesystem");
    shell_register_command("df", cmd_df, "Show disk usage");
    shell_register_command("test", cmd_test, "Run system tests");
    
    // Main shell loop
    while (1) {
        shell_prompt();
        char* line = shell_readline();
        if (!line || strlen(line) == 0) continue;
        
        char* argv[MAX_ARGS];
        int argc = shell_parse(line, argv);
        
        if (argc > 0) {
            shell_execute(argc, argv);
        }
    }
}

// Display shell prompt
void shell_prompt(void) {
    screen_set_color(COLOR_GREEN, COLOR_BLACK);
    screen_print("solixos");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_print(":");
    screen_set_color(COLOR_BLUE, COLOR_BLACK);
    screen_print(current_dir);
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_print("$ ");
}

// Read line from keyboard
char* shell_readline(void) {
    int pos = 0;
    
    while (1) {
        char c = keyboard_getchar();
        
        if (c == '\n') {
            screen_putc('\n');
            command_line[pos] = '\0';
            return command_line;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                screen_putc('\b');
            }
        } else if (c >= 32 && c < 127 && pos < MAX_CMD_LEN - 1) {
            command_line[pos++] = c;
            screen_putc(c);
        }
    }
}

// Parse command line
int shell_parse(char* line, char** argv) {
    int argc = 0;
    char* token = strtok(line, " \t");
    
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    
    return argc;
}

// Execute command
void shell_execute(int argc, char** argv) {
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            return;
        }
    }
    
    screen_print("Command not found: ");
    screen_print(argv[0]);
    screen_print("\n");
}

// Register command
void shell_register_command(const char* name, char (*func)(int, char**), const char* desc) {
    if (num_commands < 64) {
        strcpy(commands[num_commands].name, name);
        commands[num_commands].func = func;
        strcpy(commands[num_commands].description, desc);
        num_commands++;
    }
}

// Built-in command implementations

char cmd_help(int argc, char** argv) {
    screen_print("Available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        screen_print("  ");
        screen_print(commands[i].name);
        screen_print(" - ");
        screen_print(commands[i].description);
        screen_print("\n");
    }
    return 0;
}

char cmd_clear(int argc, char** argv) {
    screen_clear();
    return 0;
}

char cmd_ls(int argc, char** argv) {
    char path[MAX_PATH];
    
    if (argc == 1) {
        strcpy(path, current_dir);
    } else {
        if (argv[1][0] == '/') {
            strcpy(path, argv[1]);
        } else {
            strcpy(path, current_dir);
            if (strcmp(current_dir, "/") != 0) {
                strcat(path, "/");
            }
            strcat(path, argv[1]);
        }
    }
    
    dir_entry_t entries[64];
    int count = vfs_readdir(path, entries, 64);
    
    if (count < 0) {
        screen_print("ls: cannot access '");
        screen_print(path);
        screen_print("': No such file or directory\n");
        return 1;
    }
    
    for (int i = 0; i < count; i++) {
        if (entries[i].inode != 0) {
            screen_print(entries[i].name);
            
            // Check if it's a directory
            inode_t stat;
            char full_path[MAX_PATH];
            strcpy(full_path, path);
            if (strcmp(path, "/") != 0) {
                strcat(full_path, "/");
            }
            strcat(full_path, entries[i].name);
            
            if (vfs_stat(full_path, &stat) == 0 && stat.mode == FT_DIRECTORY) {
                screen_print("/");
            }
            
            screen_print("  ");
        }
    }
    screen_print("\n");
    
    return 0;
}

char cmd_cd(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: cd <directory>\n");
        return 1;
    }
    
    char new_path[MAX_PATH];
    
    if (argv[1][0] == '/') {
        strcpy(new_path, argv[1]);
    } else {
        strcpy(new_path, current_dir);
        if (strcmp(current_dir, "/") != 0) {
            strcat(new_path, "/");
        }
        strcat(new_path, argv[1]);
    }
    
    // Normalize path
    char* p = new_path;
    while (*p) {
        if (*p == '/' && *(p + 1) == '/') {
            memmove(p, p + 1, strlen(p));
        } else {
            p++;
        }
    }
    
    // Check if directory exists
    inode_t stat;
    if (vfs_stat(new_path, &stat) != 0 || stat.mode != FT_DIRECTORY) {
        screen_print("cd: '");
        screen_print(argv[1]);
        screen_print("': No such directory\n");
        return 1;
    }
    
    strcpy(current_dir, new_path);
    return 0;
}

char cmd_pwd(int argc, char** argv) {
    screen_print(current_dir);
    screen_print("\n");
    return 0;
}

char cmd_cat(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: cat <file>\n");
        return 1;
    }
    
    char path[MAX_PATH];
    if (argv[1][0] == '/') {
        strcpy(path, argv[1]);
    } else {
        strcpy(path, current_dir);
        if (strcmp(current_dir, "/") != 0) {
            strcat(path, "/");
        }
        strcat(path, argv[1]);
    }
    
    int fd = vfs_open(path, O_RDONLY);
    if (fd < 0) {
        screen_print("cat: '");
        screen_print(argv[1]);
        screen_print("': No such file\n");
        return 1;
    }
    
    char buffer[1024];
    ssize_t bytes_read = vfs_read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        screen_print(buffer);
    }
    
    vfs_close(fd);
    screen_print("\n");
    
    return 0;
}

char cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) screen_print(" ");
        screen_print(argv[i]);
    }
    screen_print("\n");
    return 0;
}

char cmd_mkdir(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: mkdir <directory>\n");
        return 1;
    }
    
    char path[MAX_PATH];
    if (argv[1][0] == '/') {
        strcpy(path, argv[1]);
    } else {
        strcpy(path, current_dir);
        if (strcmp(current_dir, "/") != 0) {
            strcat(path, "/");
        }
        strcat(path, argv[1]);
    }
    
    if (vfs_mkdir(path) != 0) {
        screen_print("mkdir: cannot create directory '");
        screen_print(argv[1]);
        screen_print("'\n");
        return 1;
    }
    
    return 0;
}

char cmd_touch(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: touch <file>\n");
        return 1;
    }
    
    char path[MAX_PATH];
    if (argv[1][0] == '/') {
        strcpy(path, argv[1]);
    } else {
        strcpy(path, current_dir);
        if (strcmp(current_dir, "/") != 0) {
            strcat(path, "/");
        }
        strcat(path, argv[1]);
    }
    
    int fd = vfs_open(path, O_CREAT | O_WRONLY);
    if (fd < 0) {
        screen_print("touch: cannot create file '");
        screen_print(argv[1]);
        screen_print("'\n");
        return 1;
    }
    
    vfs_close(fd);
    return 0;
}

char cmd_rm(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: rm <file>\n");
        return 1;
    }
    
    char path[MAX_PATH];
    if (argv[1][0] == '/') {
        strcpy(path, argv[1]);
    } else {
        strcpy(path, current_dir);
        if (strcmp(current_dir, "/") != 0) {
            strcat(path, "/");
        }
        strcat(path, argv[1]);
    }
    
    if (vfs_unlink(path) != 0) {
        screen_print("rm: cannot remove '");
        screen_print(argv[1]);
        screen_print("'\n");
        return 1;
    }
    
    return 0;
}

char cmd_ps(int argc, char** argv) {
    screen_print("PID  STATE  PPID  COMMAND\n");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pcb.state != PROCESS_TERMINATED) {
            screen_print_dec(process_table[i].pcb.pid);
            screen_print("   ");
            
            switch (process_table[i].pcb.state) {
                case PROCESS_RUNNING:
                    screen_print("RUN  ");
                    break;
                case PROCESS_READY:
                    screen_print("READY");
                    break;
                case PROCESS_BLOCKED:
                    screen_print("BLK  ");
                    break;
                default:
                    screen_print("UNK  ");
                    break;
            }
            
            screen_print("   ");
            screen_print_dec(process_table[i].pcb.ppid);
            screen_print("   [kernel]\n");
        }
    }
    
    return 0;
}

char cmd_kill(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: kill <pid>\n");
        return 1;
    }
    
    uint32_t pid = atoi(argv[1]);
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pcb.pid == pid && 
            process_table[i].pcb.state != PROCESS_TERMINATED) {
            process_table[i].pcb.state = PROCESS_TERMINATED;
            screen_print("Process ");
            screen_print_dec(pid);
            screen_print(" terminated\n");
            return 0;
        }
    }
    
    screen_print("Process not found: ");
    screen_print_dec(pid);
    screen_print("\n");
    return 1;
}

char cmd_reboot(int argc, char** argv) {
    screen_print("Rebooting system...\n");
    timer_wait(100);
    
    // Reboot by triple fault
    __asm__ volatile("cli");
    uint32_t* ptr = (uint32_t*)0;
    *ptr = 0;
    
    return 0;
}

char cmd_halt(int argc, char** argv) {
    screen_print("Halting system...\n");
    timer_wait(100);
    
    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
    
    return 0;
}

char cmd_meminfo(int argc, char** argv) {
    screen_print("Memory Information:\n");
    screen_print("Total frames: ");
    screen_print_dec(total_frames);
    screen_print("\n");
    screen_print("Used frames: ");
    screen_print_dec(used_frames);
    screen_print("\n");
    screen_print("Free frames: ");
    screen_print_dec(total_frames - used_frames);
    screen_print("\n");
    screen_print("Kernel heap used: ");
    screen_print_dec(kernel_heap_current - kernel_heap_start);
    screen_print(" bytes\n");
    
    return 0;
}

char cmd_mount(int argc, char** argv) {
    if (argc != 3) {
        screen_print("Usage: mount <device> <mountpoint>\n");
        return 1;
    }
    
    if (vfs_mount(argv[1], argv[2]) == 0) {
        screen_print("Mounted ");
        screen_print(argv[1]);
        screen_print(" at ");
        screen_print(argv[2]);
        screen_print("\n");
        return 0;
    } else {
        screen_print("Mount failed\n");
        return 1;
    }
}

char cmd_umount(int argc, char** argv) {
    if (argc != 2) {
        screen_print("Usage: umount <mountpoint>\n");
        return 1;
    }
    
    if (vfs_umount(argv[1]) == 0) {
        screen_print("Unmounted ");
        screen_print(argv[1]);
        screen_print("\n");
        return 0;
    } else {
        screen_print("Unmount failed\n");
        return 1;
    }
}

char cmd_df(int argc, char** argv) {
    screen_print("Filesystem    Size    Used    Available    Mount point\n");
    screen_print("SolixFS       ");
    screen_print_dec(sb.total_blocks * SOLIXFS_BLOCK_SIZE / 1024);
    screen_print("K    ");
    screen_print_dec((sb.total_blocks - sb.free_blocks) * SOLIXFS_BLOCK_SIZE / 1024);
    screen_print("K    ");
    screen_print_dec(sb.free_blocks * SOLIXFS_BLOCK_SIZE / 1024);
    screen_print("K    /\n");
    
    return 0;
}

char cmd_test(int argc, char** argv) {
    screen_print("Running system tests...\n");
    
    // Test memory allocation
    void* ptr1 = kmalloc(1024);
    void* ptr2 = kmalloc(2048);
    void* ptr3 = kmalloc(4096);
    
    if (ptr1 && ptr2 && ptr3) {
        screen_print("[+] Memory allocation test passed\n");
    } else {
        screen_print("[-] Memory allocation test failed\n");
    }
    
    kfree(ptr1);
    kfree(ptr2);
    kfree(ptr3);
    
    // Test filesystem operations
    int fd = vfs_open("/test_file", O_CREAT | O_WRONLY);
    if (fd >= 0) {
        const char* test_data = "Hello, SolixOS!";
        vfs_write(fd, test_data, strlen(test_data));
        vfs_close(fd);
        
        fd = vfs_open("/test_file", O_RDONLY);
        if (fd >= 0) {
            char buffer[32];
            ssize_t bytes = vfs_read(fd, buffer, sizeof(buffer) - 1);
            buffer[bytes] = '\0';
            vfs_close(fd);
            
            if (strcmp(buffer, test_data) == 0) {
                screen_print("[+] Filesystem test passed\n");
            } else {
                screen_print("[-] Filesystem test failed\n");
            }
            
            vfs_unlink("/test_file");
        }
    }
    
    // Test timer
    uint32_t start = timer_get_ticks();
    timer_wait(10);
    uint32_t end = timer_get_ticks();
    
    if (end - start >= 10) {
        screen_print("[+] Timer test passed\n");
    } else {
        screen_print("[-] Timer test failed\n");
    }
    
    screen_print("System tests completed\n");
    return 0;
}
