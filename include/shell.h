#ifndef SOLIX_SHELL_H
#define SOLIX_SHELL_H

#include "types.h"

// Shell constants
#define MAX_CMD_LEN 256
#define MAX_ARGS 16
#define MAX_PATH 512

// Command structure
typedef struct command {
    char name[32];
    char (*func)(int argc, char** argv);
    char description[128];
} command_t;

// Shell functions
void shell_start(void);
void shell_prompt(void);
char* shell_readline(void);
int shell_parse(char* line, char** argv);
void shell_execute(int argc, char** argv);
void shell_register_command(const char* name, char (*func)(int, char**), const char* desc);

// Built-in commands
char cmd_help(int argc, char** argv);
char cmd_clear(int argc, char** argv);
char cmd_ls(int argc, char** argv);
char cmd_cd(int argc, char** argv);
char cmd_pwd(int argc, char** argv);
char cmd_cat(int argc, char** argv);
char cmd_echo(int argc, char** argv);
char cmd_mkdir(int argc, char** argv);
char cmd_touch(int argc, char** argv);
char cmd_rm(int argc, char** argv);
char cmd_ps(int argc, char** argv);
char cmd_kill(int argc, char** argv);
char cmd_reboot(int argc, char** argv);
char cmd_halt(int argc, char** argv);
char cmd_meminfo(int argc, char** argv);
char cmd_mount(int argc, char** argv);
char cmd_umount(int argc, char** argv);
char cmd_df(int argc, char** argv);
char cmd_test(int argc, char** argv);

#endif
