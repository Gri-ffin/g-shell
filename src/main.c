#include "builtins.h"
#include "parser.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 64

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    system("clear");
    char *input = NULL;
    size_t cap = 0;

    while (true) {
        printf("$ ");

        const ssize_t len = getline(&input, &cap, stdin);
        if (len == -1) {
            printf("Exiting Shell...\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        char *args[MAX_ARGS];

        const int arg_count = parse_input(input, args);
        if (arg_count == 0)
            continue;

        // guard against overflow
        if (arg_count >= MAX_ARGS - 1) {
            fprintf(stderr, "too many arguments\n");
            continue;
        }

        const char *cmd = args[0];
        int target_stream = STDOUT_FILENO;
        const int fd = check_and_handle_redirection(args, &target_stream);

        // if we encountered an error
        if (fd == -1)
            continue;

        int saved_stderr = -1;
        // don't overwrite the program main stderr
        if (target_stream == STDERR_FILENO) {
            saved_stderr = dup(STDERR_FILENO);
            dup2(fd, STDERR_FILENO);
        }

        const int pass_fd = (target_stream == STDOUT_FILENO) ? fd : STDOUT_FILENO;
        if (strcmp(cmd, "exit") == 0) {
            exit(0);
        }
        if (strcmp(cmd, "echo") == 0) {
            handle_echo(args, pass_fd);
        } else if (strcmp(cmd, "type") == 0) {
            handle_type(args[1], pass_fd);
        } else if (strcmp(cmd, "pwd") == 0) {
            handle_pwd(pass_fd);
        } else if (strcmp(cmd, "cd") == 0) {
            handle_cd(args[1]);
        } else {
            run_program(cmd, args, pass_fd);
        }

        // restore the main stderr
        if (saved_stderr != -1) {
            dup2(saved_stderr, STDERR_FILENO);
            close(saved_stderr);
        }
        // don't accidentally close the main streams
        if (fd != STDOUT_FILENO && fd != STDERR_FILENO)
            close(fd);
    }

    free(input);
    return 0;
}
