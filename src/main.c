#include "builtins/builtins.h"
#include "parser.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include "builtins/autocompletion.h"
#include "command.h"

int main() {
    setbuf(stdout, NULL);
    printf("\033[H\033[2J");
    char *input = NULL;

    rl_attempted_completion_function = shell_completion;

    while (true) {
        input = readline("$ ");

        if (input == NULL) {
            break;
        }

        Command cmd = parse_command(input);
        if (cmd.arg_count == 0) {
            free(input);
            continue;
        }

        // guard against overflow
        if (cmd.arg_count >= MAX_ARGS - 1) {
            fprintf(stderr, "too many arguments\n");
            free(input);
            continue;
        }

        if (cmd.fd_out != STDOUT_FILENO) {
            cmd.saved_stdout = dup(STDOUT_FILENO);
            dup2(cmd.fd_out, STDOUT_FILENO);
        }

        if (cmd.fd_err != STDERR_FILENO) {
            cmd.saved_stderr = dup(STDERR_FILENO);
            dup2(cmd.fd_err, STDERR_FILENO);
        }

        if (strcmp(cmd.cmd, "exit") == 0) {
            free(input);
            exit(0);
        }
        const builtin_fn fn = find_builtin(cmd.cmd);
        if (fn) fn(&cmd);
        else run_program(cmd.cmd, cmd.args, STDOUT_FILENO);

        if (cmd.saved_stdout != -1) {
            dup2(cmd.saved_stdout, STDOUT_FILENO); // Put the terminal back
            close(cmd.saved_stdout); // Close the duplicate
            close(cmd.fd_out); // Close the file we wrote to
        }

        // Restore standard error if we altered it
        if (cmd.saved_stderr != -1) {
            dup2(cmd.saved_stderr, STDERR_FILENO); // Put the terminal back
            close(cmd.saved_stderr); // Close the duplicate
            close(cmd.fd_err); // Close the file we wrote to
        }

        free(input);
    }

    return 0;
}
