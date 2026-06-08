#ifndef COMMAND_H
#define COMMAND_H

#define MAX_ARGS 64

typedef struct {
    char *args[MAX_ARGS];
    int arg_count;
    char *cmd;

    // I/O Redirection state
    int fd_out;
    int fd_err;
    int saved_stdout;
    int saved_stderr;
} Command;
#endif // COMMAND_H
