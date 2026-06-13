#include "builtins.h"
#include "../path.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *builtins[] = {"complete", "exit", "go", "print", "pwd", "whatis", NULL};
const int builtins_count = sizeof(builtins) / sizeof(*builtins) - 1;
/**
 * @brief prints the arguments back to the user
 * @param args the arguments passed in the shell
 */
void handle_print(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf(" ");
        }
    }
    printf("\n");
}

/**
 * @brief prints whether the argument is a valid builtin
 * or an external program or doesn't exist.
 * @param arg the argument to handle type onto
 */
void handle_whatis(char *arg) {
    int i = 0;
    if (arg == NULL) {
        printf("type: missing argument\n");
        return;
    }

    while (builtins[i] != NULL) {
        if (strcmp(builtins[i], arg) == 0) {
            printf("%s is a shell builtin\n", arg);
            return;
        }
        i++;
    }

    char resolved_path[PATH_MAX];
    if (resolve_path(arg, resolved_path, sizeof(resolved_path))) {
        printf("%s is %s\n", arg, resolved_path);
    } else {
        printf("%s: not found\n", arg);
    }
}

/**
 * @brief Prints the current working directory.
 */
void handle_pwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd failed\n");
    }
}

/**
 * @brief Changes the current working directory, defaulting to HOME if empty or '~'.
 * @param path The destination directory path.
 */
void handle_go(const char *path) {
    if (path == NULL || strcmp(path, "~") == 0) {
        const char *home = getenv("HOME");

        if (home == NULL) {
            fprintf(stderr, "go: environment variable home not set.\n");
            return;
        }

        if (chdir(home) == -1) {
            perror("go");
        }
        return;
    }

    if (chdir(path) == -1) {
        fprintf(stderr, "go: %s: %s\n", path, strerror(errno));
    }
}


/**
 * @brief Forks and executes an external system program with optional I/O redirection.
 * @param program  The name or path of the external command.
 * @param args     The null-terminated argument vector for the command.
 */
void run_program(const char *program, char **args) {
    char resolved_path[PATH_MAX];
    if (resolve_path(program, resolved_path, sizeof(resolved_path))) {
        const pid_t pid = fork();
        if (pid == -1) {
            perror("failed to create the fork");
            return;
        }

        if (pid == 0) {
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

static void do_print(Command *cmd) { handle_print(cmd->args); }
static void do_whatis(Command *cmd) { handle_whatis(cmd->args[1]); }
static void do_pwd(Command *cmd) { handle_pwd(); }
static void do_go(Command *cmd) { handle_go(cmd->args[1]); }
static void do_complete(Command *cmd) { handle_complete(cmd->args, cmd->arg_count); }

static const BuiltinEntry dispatch[] = {
    {"complete", do_complete},
    {"print", do_print},
    {"whatis", do_whatis},
    {"pwd", do_pwd},
    {"go", do_go},
    {NULL, NULL}, // sentinel
};

builtin_fn find_builtin(const char *name) {
    for (int i = 0; dispatch[i].name != NULL; i++) {
        if (strcmp(dispatch[i].name, name) == 0)
            return dispatch[i].fn;
    }
    return NULL;
}
