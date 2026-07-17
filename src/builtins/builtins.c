#define _POSIX_C_SOURCE 200112L

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
#include "complete.h"
#include "jobs.h"

#define COMMAND_NOT_FOUND 127
#define BUFF_SIZE 256

char *builtins[] = {"complete", "exit", "go", "history", "jobs", "print", "pwd", "whatis", NULL};
const int builtins_count = (sizeof(builtins) / sizeof(*builtins)) - 1;
extern DynamicArray *history_array;


/**
 * @brief prints the arguments back to the user
 * @param args the arguments passed in the shell
 */
static int handle_print(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf(" ");
        }
    }
    printf("\n");
    return EXIT_SUCCESS;
}

/**
 * @brief prints whether the argument is a valid builtin
 * or an external program or doesn't exist.
 * @param arg the argument to handle type onto
 */
static int handle_whatis(char *arg) {
    int index = 0;
    if (arg == NULL) {
        printf("type: missing argument\n");
        return EXIT_FAILURE;
    }

    while (builtins[index] != NULL) {
        if (strcmp(builtins[index], arg) == 0) {
            printf("%s is a shell builtin\n", arg);
            return EXIT_FAILURE;
        }
        index++;
    }

    char resolved_path[PATH_MAX];
    if (resolve_path(arg, resolved_path, sizeof(resolved_path))) {
        printf("%s is %s\n", arg, resolved_path);
    } else {
        printf("%s: not found\n", arg);
    }
    return EXIT_SUCCESS;
}

/**
 * @brief Prints the current working directory.
 */
int handle_pwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return EXIT_SUCCESS;
    }
    perror("pwd failed\n");
    return EXIT_FAILURE;
}

/**
 * @brief Changes the current working directory, defaulting to HOME if empty or '~'.
 * @param path The destination directory path.
 */
static int handle_go(const char *path) {
    if (path == NULL || strcmp(path, "~") == 0) {
        const char *home = getenv("HOME"); // NOLINT(concurrency-mt-unsafe);

        if (home == NULL) {
            (void) fprintf(stderr, "go: environment variable home not set.\n");
            return EXIT_FAILURE;
        }

        if (chdir(home) == -1) {
            perror("go");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    if (chdir(path) == -1) {
        char errbuf[BUFF_SIZE];
        if (strerror_r(errno, errbuf, sizeof(errbuf)) == 0) {
            (void) fprintf(stderr, "go: %s: %s\n", path, errbuf);
        } else {
            (void) fprintf(stderr, "go: %s: unknown error (%d)\n", path, errno);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Handles the printing of the jobs that are running
 */
void handle_jobs() {
    jobs_print();
}

/**
 *
 * @brief Prints the history of commands typed
 */
static void handle_history(char *arg) {
    if (history_array == NULL || history_array->count == 0) {
        printf("You still didn't type any command :<\n");
        return;
    }
    int count;
    if (arg == NULL) {
        count = history_array->count;
    } else {
        const int parsed = atoi(arg);
        if (parsed < 0) count = 0;
        else if (parsed >= history_array->count) {
            count = history_array->count;
        } else {
            count = parsed;
        }
    }
    for (int i = 0; i < count; i++) {
        printf("\t %d %s\n", i + 1, (char *) history_array->items[i]);
    }
}

/**
 * @brief Forks and executes an external system program with optional I/O redirection.
 * @param program  The name or path of the external command.
 * @param args     The null-terminated argument vector for the command.
 */
int run_program(const char *program, char **args) {
    char resolved_path[PATH_MAX];
    if (resolve_path(program, resolved_path, sizeof(resolved_path))) {
        const pid_t pid = fork();
        if (pid == -1) {
            perror("failed to create the fork");
            return EXIT_FAILURE;
        }

        if (pid == 0) {
            execv(resolved_path, args);
            perror("execv failed");
            exit(1);
        }
        int status;
        waitpid(pid, &status, 0);

        // return exit code for the program
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return EXIT_FAILURE;
    }
    printf("%s: command not found\n", program);
    return COMMAND_NOT_FOUND;
}

static int do_print(Command *cmd) {
    handle_print(cmd->args);
    return EXIT_SUCCESS;
}

static int do_whatis(Command *cmd) { return handle_whatis(cmd->args[1]); }
static int do_pwd(Command *cmd) { return handle_pwd(); }
static int do_go(Command *cmd) { return handle_go(cmd->args[1]); }
static int do_complete(Command *cmd) { return handle_complete(cmd->args, cmd->arg_count); }

static int do_jobs(Command *cmd) {
    handle_jobs();
    return EXIT_SUCCESS;
}


static int do_history(Command *cmd) {
    handle_history(cmd->args[1]);
    return EXIT_SUCCESS;
}

static const BuiltinEntry dispatch[] = {
    {"complete", do_complete},
    {"print", do_print},
    {"whatis", do_whatis},
    {"pwd", do_pwd},
    {"go", do_go},
    {"jobs", do_jobs},
    {"history", do_history},
    {NULL, NULL}, // sentinel
};

builtin_fn find_builtin(const char *name) {
    for (int i = 0; dispatch[i].name != NULL; i++) {
        if (strcmp(dispatch[i].name, name) == 0)
            return dispatch[i].fn;
    }
    return NULL;
}
