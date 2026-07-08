#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/readline.h>

#include "builtins/auto_completion/autocompletion.h"
#include "builtins/builtins.h"
#include "builtins/jobs.h"
#include "command.h"
#include "parser.h"

// THESE SHOULD NEVER GO TO THE BACKGROUND JOB
static const char *const no_background[] = {"go", "exit", "cd", NULL};

/**
 *
 * @param cmd command name
 * @return if the command can be run in the background or not
 */
static bool can_background(const char *cmd) {
    for (int i = 0; no_background[i]; i++)
        if (strcmp(cmd, no_background[i]) == 0)
            return false;
    return true;
}

/**
 * @brief frees the chain of commands
 * @param head the start of the linked list of commands to free
 */
static void free_command_chain(const Command *head) {
    Command *node_to_free = head->next;
    while (node_to_free != NULL) {
        Command *tmp = node_to_free->next;
        free(node_to_free);
        node_to_free = tmp;
    }
}

/**
 * @brief sets up piping and standard error/output/input redirection
 * @param cmd the command name
 * @param pipefd the tunnel of write in/out to pipe
 * @param prev_pipe_read the previous pipe output to read from in new stdin
 * returns saved standard input
 */
static int setup_pipes_and_redirects(Command *cmd, int pipefd[2],
                                     const int *prev_pipe_read) {
    if (cmd->op == PIPE_OP) {
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return -1;
        }
        if (cmd->fd_out == STDOUT_FILENO) {
            cmd->fd_out = pipefd[1];
        } else {
            close(pipefd[1]);
        }
    }

    int saved_stdin = -1;
    if (*prev_pipe_read != -1) {
        saved_stdin = dup(STDIN_FILENO);
        dup2(*prev_pipe_read, STDIN_FILENO);
        close(*prev_pipe_read);
    }

    if (cmd->fd_out != STDOUT_FILENO) {
        cmd->saved_stdout = dup(STDOUT_FILENO);
        dup2(cmd->fd_out, STDOUT_FILENO);
    }

    if (cmd->fd_err != STDERR_FILENO) {
        cmd->saved_stderr = dup(STDERR_FILENO);
        dup2(cmd->fd_err, STDERR_FILENO);
    }

    return saved_stdin;
}

/**
 * @brief restores the overwritten file descriptors to default vals
 * @param cmd the command name
 * @param saved_stdin the standard stdin we saved to restore
 */
static void restore_std_fds(const Command *cmd, const int saved_stdin) {
    if (cmd->saved_stdout != -1) {
        dup2(cmd->saved_stdout, STDOUT_FILENO);
        close(cmd->saved_stdout);
        close(cmd->fd_out);
    }

    if (cmd->saved_stderr != -1) {
        dup2(cmd->saved_stderr, STDERR_FILENO);
        close(cmd->saved_stderr);
        close(cmd->fd_err);
    }

    if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
}

/**
 * @brief dispatch the command to its appropriate function
 * @param cmd the command name
 * returns exit code, 0 on success, otherwise its an error
 */
static int dispatch_command(Command *cmd) {
    const builtin_fn builtin_function = find_builtin(cmd->cmd);

    if (cmd->background && can_background(cmd->cmd)) {
        const pid_t pid = fork();
        if (pid == 0) {
            if (builtin_function) {
                builtin_function(cmd);
                _exit(0);
            }
            run_program(cmd->cmd, cmd->args);
            _exit(0);
        }
        jobs_add(pid, cmd->cmd);
        return 0;
    }

    if (builtin_function)
        return builtin_function(cmd);

    return run_program(cmd->cmd, cmd->args);
}

static void execute_command_chain(Command *head, char *raw_input) {
    Command *current = head;
    int last_status = 0;
    int prev_pipe_read = -1;

    while (current != NULL) {
        int pipefd[2] = {-1, -1};
        int saved_stdin =
                setup_pipes_and_redirects(current, pipefd, &prev_pipe_read);

        if (strcmp(current->cmd, "exit") == 0) {
            free(raw_input);
            exit(0);
        }

        last_status = dispatch_command(current);
        restore_std_fds(current, saved_stdin);

        if (current->op == AND_OP && last_status != 0) {
            if (pipefd[0] != -1)
                close(pipefd[0]);
            break;
        }

        if ((current->op == AND_OP || current->op == PIPE_OP) &&
            current->next != NULL && current->next->arg_count == 0) {
            (void) fprintf(stderr, "the chaining command wasn't given.\n");
            if (pipefd[0] != -1)
                close(pipefd[0]);
            break;
        }

        prev_pipe_read = pipefd[0];
        current = current->next;
    }
}

int main() {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
    }
    printf("\033[H\033[2J");

    rl_attempted_completion_function = shell_completion;
    char *input = NULL;

    while (true) {
        jobs_reap();
        input = readline("$ ");

        if (fflush(stdout) != 0) {
            perror("fflush");
        }

        if (input == NULL)
            break;

        Command cmd = parse_command(input);

        if (cmd.arg_count == 0) {
            free(input);
            continue;
        }

        if (cmd.arg_count >= MAX_ARGS - 1) {
            (void) fprintf(stderr, "too many arguments\n");
            free(input);
            continue;
        }

        execute_command_chain(&cmd, input);
        free_command_chain(&cmd);
        free(input);
    }

    return 0;
}
