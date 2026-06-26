#include "builtins/builtins.h"
#include "parser.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include "builtins/auto_completion/autocompletion.h"
#include "command.h"
#include "builtins/jobs.h"

// THESE SHOULD NEVER GO TO THE BACKGROUND JOB
static const char *no_background[] = {"go", "exit", "cd", NULL};

/**
 *
 * @param cmd command name
 * @return if the command can be run in the background or not
 */
static bool can_background(const char *cmd) {
    for (int i = 0; no_background[i]; i++)
        if (strcmp(cmd, no_background[i]) == 0) return false;
    return true;
}

int main() {
    setbuf(stdout, NULL);
    printf("\033[H\033[2J");
    char *input = NULL;

    rl_attempted_completion_function = shell_completion;

    while (true) {
        jobs_reap();
        input = readline("$ ");
        fflush(stdout);

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

        Command *current_command = &cmd;
        int last_status = 0;
        while (current_command != NULL) {
            // TODO: handle chaining pipe operator
            if (current_command->fd_out != STDOUT_FILENO) {
                current_command->saved_stdout = dup(STDOUT_FILENO);
                dup2(current_command->fd_out, STDOUT_FILENO);
            }

            if (current_command->fd_err != STDERR_FILENO) {
                current_command->saved_stderr = dup(STDERR_FILENO);
                dup2(current_command->fd_err, STDERR_FILENO);
            }

            if (strcmp(current_command->cmd, "exit") == 0) {
                free(input);
                exit(0);
            }
            const builtin_fn fn = find_builtin(current_command->cmd);
            if (current_command->background && can_background(current_command->cmd)) {
                const pid_t pid = fork();
                if (pid == 0) {
                    if (fn) {
                        fn(current_command);
                        exit(0);
                    }
                    run_program(current_command->cmd, current_command->args);
                    exit(0);
                }
                jobs_add(pid, current_command->cmd);
                last_status = 0;
            } else {
                if (fn) last_status = fn(current_command);
                else last_status = run_program(current_command->cmd, current_command->args);
            }

            if (current_command->saved_stdout != -1) {
                dup2(current_command->saved_stdout, STDOUT_FILENO); // Put the terminal back
                close(current_command->saved_stdout); // Close the duplicate
                close(current_command->fd_out); // Close the file we wrote to
            }

            // Restore standard error if we altered it
            if (current_command->saved_stderr != -1) {
                dup2(current_command->saved_stderr, STDERR_FILENO); // Put the terminal back
                close(current_command->saved_stderr); // Close the duplicate
                close(current_command->fd_err); // Close the file we wrote to
            }

            if (current_command->op == AND_OP && last_status != 0) {
                break;
            }
            if ((current_command->op == AND_OP || current_command->op == PIPE_OP) &&
                current_command->next->arg_count == 0) {
                fprintf(stderr, "the chaining command wasn't given.\n");
                break;
            }

            current_command = current_command->next;
        }
        // Free the dynamically allocated parts of the command chain
        // We skip the head node (&cmd) because it's allocated on the stack!
        Command *node_to_free = cmd.next;
        while (node_to_free != NULL) {
            Command *tmp = node_to_free->next;
            free(node_to_free);
            node_to_free = tmp;
        }

        free(input);
    }

    return 0;
}
