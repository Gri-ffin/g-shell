#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STDOUT_REDIRECTION_SHORT ">"
#define STDOUT_REDIRECTION "1>"
#define STDERR_REDIRECTION "2>"
#define STDOUT_APPEND_REDIRECTION_SHORT ">>"
#define STDOUT_APPEND_REDIRECTION "1>>"
#define STDERR_APPEND_REDIRECTION "2>>"
#define FILE_ERROR (-2)
#define NO_REDIRECTION (-1)
#define REDIRECT_ERROR (-1)

// handles the arguments parsing including single quotes, double quotes etc.
int parse_input(char *input, char **args) {
    bool inside_single_quotes = false;
    bool inside_double_quotes = false;
    char *start_arg = NULL;
    char *ptr = input;
    int arg_count = 0;

    while (*ptr != '\0') {
        if (*ptr == '\'' && !inside_double_quotes) {
            inside_single_quotes = !inside_single_quotes;
            // skip the single quote
            memmove(ptr, ptr + 1, strlen(ptr));
            // if not the start of an arg, mark this as the start
            if (!start_arg) {
                start_arg = ptr;
            }
            continue;
        }
        if (*ptr == '"' && !inside_single_quotes) {
            inside_double_quotes = !inside_double_quotes;
            // skip the double quote
            memmove(ptr, ptr + 1, strlen(ptr));
            // if not the start of an arg, mark this as the start
            if (!start_arg) {
                start_arg = ptr;
            }
            continue;
        }
        if (*ptr == '\\' && !inside_double_quotes && !inside_single_quotes) {
            // skip the slash
            memmove(ptr, ptr + 1, strlen(ptr));
            // if we didn't hit end of string, mark this as start and continue
            if (*ptr != '\0') {
                if (!start_arg) {
                    start_arg = ptr;
                }
                ptr++;
            }
            continue;
        }

        if (*ptr == ' ' && !inside_single_quotes && !inside_double_quotes) {
            if (start_arg) {
                // mark this as the end of the argument
                *ptr = '\0';
                args[arg_count++] = start_arg;
                start_arg = NULL;
            }
            // skip the empty spaces otherwise
        } else {
            // if normal char mark is as start if not already done so
            if (!start_arg) {
                start_arg = ptr;
            }
        }
        ptr++;
    }

    // if we have trailing quotes, error instead of the annoying quote> or dquote>
    if (inside_double_quotes || inside_single_quotes) {
        fprintf(stderr, "syntax error: unterminated quote\n");
        return 0;
    }

    // if trailing args handle them
    if (start_arg) {
        args[arg_count++] = start_arg;
    }
    args[arg_count] = NULL;
    return arg_count;
}

// handle fd and stdout or stderr output
// return -1 on skip, on error `FILE_ERROR`
int parse_redirection(const char *arg, int *target_stream, const char *filename) {
    *target_stream = STDOUT_FILENO;

    const bool is_stdout = strcmp(arg, STDOUT_REDIRECTION) == 0 ||
                           strcmp(arg, STDOUT_REDIRECTION_SHORT) == 0 ||
                           strcmp(arg, STDOUT_APPEND_REDIRECTION_SHORT) == 0 ||
                           strcmp(arg, STDOUT_APPEND_REDIRECTION) == 0;

    const bool is_stderr = strcmp(arg, STDERR_REDIRECTION) == 0 || strcmp(arg, STDERR_APPEND_REDIRECTION) == 0;

    const bool is_append = strcmp(arg, STDOUT_APPEND_REDIRECTION) == 0 ||
                           strcmp(arg, STDOUT_APPEND_REDIRECTION_SHORT) == 0 ||
                           strcmp(arg, STDERR_APPEND_REDIRECTION) == 0;

    if (is_stdout || is_stderr) {
        if (is_stdout)
            *target_stream = STDOUT_FILENO;
        else
            *target_stream = STDERR_FILENO;

        if (filename == NULL) {
            fprintf(stderr, "Shell error: No syntax file specified\n");
            return FILE_ERROR;
        }

        int flags = O_WRONLY | O_CREAT;
        if (is_append) {
            flags |= O_APPEND;
        } else {
            flags |= O_TRUNC;
        }
        const int fd_file = open(filename, flags, 0644);
        if (fd_file == -1) {
            perror(filename);
            return FILE_ERROR;
        }
        return fd_file;
    }

    return NO_REDIRECTION;
}

// checks if there is any redirection involved like '>' or '>>'
int check_and_handle_redirection(char **args, int *target_stream) {
    for (int i = 1; args[i] != NULL; i++) {
        const int fd = parse_redirection(args[i], target_stream, args[i + 1]);
        // if file error return REDIRECT_ERROR
        if (fd == FILE_ERROR)
            return REDIRECT_ERROR;
        // if we have file redirection crop the redirection token and file
        if (fd != NO_REDIRECTION) {
            int j = i;
            while (args[j + 2] != NULL) {
                args[j] = args[j + 2];
                j++;
            }
            args[j] = NULL;
            return fd;
        }
    }

    return STDOUT_FILENO;
}
