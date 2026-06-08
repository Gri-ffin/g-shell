#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

// Helper to extract redirections from the arguments list and open the files
// Returns 0 on success, -1 on error
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

// Main parser function
Command parse_command(char *input) {
    Command cmd;
    // Initialize the struct with safe defaults
    memset(&cmd, 0, sizeof(Command));
    cmd.fd_out = STDOUT_FILENO;
    cmd.fd_err = STDERR_FILENO;
    cmd.saved_stdout = -1;
    cmd.saved_stderr = -1;

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

        if (*ptr == ' ' && !inside_single_quotes && !inside_double_quotes) {
            if (start_arg) {
                *ptr = '\0';
                cmd.args[cmd.arg_count++] = start_arg;
                start_arg = NULL;
            }
        } else {
            if (!start_arg) start_arg = ptr;
        }
        ptr++;
    }

    if (inside_double_quotes || inside_single_quotes) {
        fprintf(stderr, "syntax error: unterminated quote\n");
        cmd.arg_count = PARSING_ERROR;
        return cmd;
    }

    if (start_arg) {
        cmd.args[cmd.arg_count++] = start_arg;
    }
    cmd.args[cmd.arg_count] = NULL;

    // If we successfully parsed arguments, resolve redirections and set the main command
    if (cmd.arg_count > 0) {
        if (process_redirections(&cmd) == -1) {
            cmd.arg_count = PARSING_ERROR;
        } else {
            cmd.cmd = cmd.args[0]; // Set the command
        }
    }

    return cmd;
}
