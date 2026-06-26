#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <_locale_posix2008.h>

#include "command.h"

#define STDOUT_REDIRECTION_SHORT ">"
#define STDOUT_REDIRECTION "1>"
#define STDERR_REDIRECTION "2>"
#define STDOUT_APPEND_REDIRECTION_SHORT ">>"
#define STDOUT_APPEND_REDIRECTION "1>>"
#define STDERR_APPEND_REDIRECTION "2>>"
#define FILE_ERROR (-2)
#define NO_REDIRECTION (-1)
#define REDIRECT_ERROR (-1)
#define REDIRECTION_SUCCESS (0)
#define PARSING_ERROR (-1)

/**
 * @brief Scans arguments for I/O redirection operators, opens target files, and removes tokens.
 * @param cmd Pointer to the Command structure to modify.
 * @return 0 (REDIRECTION_SUCCESS) on success, or negative error code on failure.
 */
static int process_redirections(Command *cmd) {
    for (int i = 0; i < cmd->arg_count; i++) {
        if (cmd->args[i] == NULL) break;

        const char *arg = cmd->args[i];

        // Group the redirection types logically
        const bool is_stdout = (strcmp(arg, ">") == 0 || strcmp(arg, "1>") == 0 || strcmp(arg, ">>") == 0 ||
                                strcmp(arg, "1>>") == 0);
        const bool is_stderr = (strcmp(arg, "2>") == 0 || strcmp(arg, "2>>") == 0);
        const bool is_append = (strcmp(arg, ">>") == 0 || strcmp(arg, "1>>") == 0 || strcmp(arg, "2>>") == 0);

        if (is_stdout || is_stderr) {
            const char *filename = cmd->args[i + 1];

            if (filename == NULL) {
                fprintf(stderr, "syntax error: expected file after redirection\n");
                return REDIRECT_ERROR;
            }

            const int flags = O_WRONLY | O_CREAT | (is_append ? O_APPEND : O_TRUNC);
            const int fd = open(filename, flags, 0644);
            if (fd == -1) {
                perror(filename);
                return FILE_ERROR;
            }

            // Assign the file descriptor to the correct stream in the struct
            if (is_stdout) cmd->fd_out = fd;
            if (is_stderr) cmd->fd_err = fd;

            // Crop the redirection token and the filename out of the args array
            for (int j = i; j < cmd->arg_count - 1; j++) {
                cmd->args[j] = cmd->args[j + 2];
            }

            cmd->arg_count -= 2;
            cmd->args[cmd->arg_count] = NULL; // Ensure null termination
            i--; // Re-check the current index since we shifted the array left
        }
    }
    return REDIRECTION_SUCCESS;
}

/**
 * @brief Parses a raw input string into a shell Command structure, handling quotes, escapes, and redirections.
 * @param input The raw command line string to be tokenized and parsed.
 * @return A populated Command structure; cmd.arg_count will equal PARSING_ERROR on failure.
 */
Command parse_command(char *input) {
    Command head_cmd;
    // Initialize the struct with safe defaults
    memset(&head_cmd, 0, sizeof(Command));
    head_cmd.fd_out = STDOUT_FILENO;
    head_cmd.fd_err = STDERR_FILENO;
    head_cmd.saved_stdout = -1;
    head_cmd.saved_stderr = -1;

    Command *cur_cmd = &head_cmd;
    bool inside_single_quotes = false;
    bool inside_double_quotes = false;
    char *start_arg = NULL;
    char *ptr = input;

    // Tokenize the input
    while (*ptr != '\0') {
        if (*ptr == '\'' && !inside_double_quotes) {
            inside_single_quotes = !inside_single_quotes;
            memmove(ptr, ptr + 1, strlen(ptr));
            if (!start_arg) start_arg = ptr;
            continue;
        }
        if (*ptr == '"' && !inside_single_quotes) {
            inside_double_quotes = !inside_double_quotes;
            memmove(ptr, ptr + 1, strlen(ptr));
            if (!start_arg) start_arg = ptr;
            continue;
        }
        if (*ptr == '\\' && !inside_double_quotes && !inside_single_quotes) {
            memmove(ptr, ptr + 1, strlen(ptr));
            if (*ptr != '\0') {
                if (!start_arg) start_arg = ptr;
                ptr++;
            }
            continue;
        }

        if (!inside_double_quotes && !inside_single_quotes && strncmp(ptr, "&&", 2) == 0) {
            // Cap off the current arg if we are building one
            if (start_arg) {
                *ptr = '\0';
                cur_cmd->args[cur_cmd->arg_count++] = start_arg;
                start_arg = NULL;
            }

            // finalize the current command args
            cur_cmd->args[cur_cmd->arg_count] = NULL;
            cur_cmd->op = AND_OP; // chain

            cur_cmd->next = calloc(1, sizeof(Command));
            cur_cmd = (Command *) cur_cmd->next;

            // Initialize defaults
            cur_cmd->fd_out = STDOUT_FILENO;
            cur_cmd->fd_err = STDERR_FILENO;
            cur_cmd->saved_stdout = -1;
            cur_cmd->saved_stderr = -1;

            // don't forget to skip &&
            *ptr = '\0';
            ptr += 2;
            continue;
        }

        if (!inside_double_quotes && !inside_single_quotes && strncmp(ptr, "|", 1) == 0) {
            // Cap of the current arg if we are building one
            if (start_arg) {
                *ptr = '\0';
                cur_cmd->args[cur_cmd->arg_count++] = start_arg;
                start_arg = NULL;
            }

            // finalize the current command args
            cur_cmd->args[cur_cmd->arg_count] = NULL;
            cur_cmd->op = PIPE_OP; // chain

            cur_cmd->next = calloc(1, sizeof(Command));
            cur_cmd = (Command *) cur_cmd->next;

            // Initialize defaults
            cur_cmd->fd_out = STDOUT_FILENO;
            cur_cmd->fd_err = STDERR_FILENO;
            cur_cmd->saved_stdout = -1;
            cur_cmd->saved_stderr = -1;

            // don't forget to skip |
            *ptr = '\0';
            ptr += 1;
            continue;
        }

        if (*ptr == ' ' && !inside_single_quotes && !inside_double_quotes) {
            if (start_arg) {
                *ptr = '\0';
                cur_cmd->args[cur_cmd->arg_count++] = start_arg;
                start_arg = NULL;
            }
        } else {
            if (!start_arg) start_arg = ptr;
        }
        ptr++;
    }

    if (inside_double_quotes || inside_single_quotes) {
        fprintf(stderr, "syntax error: unterminated quote\n");
        head_cmd.arg_count = PARSING_ERROR;
        return head_cmd;
    }

    if (start_arg) {
        cur_cmd->args[cur_cmd->arg_count++] = start_arg;
    }
    cur_cmd->args[cur_cmd->arg_count] = NULL;

    cur_cmd = &head_cmd;
    while (cur_cmd != NULL) {
        // we have a job we need to run
        if (cur_cmd->arg_count > 0 && strcmp(cur_cmd->args[cur_cmd->arg_count - 1], "&") == 0) {
            cur_cmd->background = true;
            cur_cmd->args[--cur_cmd->arg_count] = NULL;
        }

        // If we successfully parsed arguments, resolve redirections and set the main command
        if (cur_cmd->arg_count > 0) {
            if (process_redirections(cur_cmd) == -1) {
                cur_cmd->arg_count = PARSING_ERROR;
            } else {
                cur_cmd->cmd = cur_cmd->args[0]; // Set the command
            }
        }
        cur_cmd = (Command *) cur_cmd->next;
    }

    return head_cmd;
}
