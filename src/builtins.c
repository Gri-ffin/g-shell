#include "builtins.h"
#include "path.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *builtins[] = {"echo", "exit", "type", "cd", "pwd"};

// The `echo` builtin command
void handle_echo(char **args, const int fd) {
    for (int i = 1; args[i] != NULL; i++) {
        dprintf(fd, "%s", args[i]);
        if (args[i + 1] != NULL) {
            dprintf(fd, " ");
        }
    }
    dprintf(fd, "\n");
}

// The `type` builtin command
void handle_type(char *arg, const int fd) {
    if (arg == NULL) {
        dprintf(fd, "type: missing argument\n");
        return;
    }

    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(builtins[i], arg) == 0) {
            dprintf(fd, "%s is a shell builtin\n", arg);
            return;
        }
    }

    char resolved_path[PATH_MAX];
    if (resolve_path(arg, resolved_path, sizeof(resolved_path))) {
        dprintf(fd, "%s is %s\n", arg, resolved_path);
    } else {
        dprintf(fd, "%s: not found\n", arg);
    }
}

// The `pwd` builtin comamnd
void handle_pwd(const int fd) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        dprintf(fd, "%s\n", cwd);
    } else {
        perror("pwd failed\n");
    }
}

// The `cd` builtin command
void handle_cd(const char *path) {
    if (path == NULL || strcmp(path, "~") == 0) {
        const char *home = getenv("HOME");

        if (home == NULL) {
            fprintf(stderr, "cd: environment variable home not set.\n");
            return;
        }

        if (chdir(home) == -1) {
            perror("cd");
        }
        return;
    }

    if (chdir(path) == -1) {
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
    }
}

// Function to run external commands
void run_program(const char *program, char **args, const int fd) {
    char resolved_path[PATH_MAX];
    if (resolve_path(program, resolved_path, sizeof(resolved_path))) {
        const pid_t pid = fork();
        if (pid == -1) {
            perror("failed to create the fork");
            return;
        }

        if (pid == 0) {
            // for redirection force standard output to the new file descriptor instead
            if (fd != STDOUT_FILENO) {
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            execv(resolved_path, args);
            perror("execv failed");
            exit(1);
        }
        int status;
        waitpid(pid, &status, 0);
    } else {
        printf("%s: command not found\n", program);
    }
}
