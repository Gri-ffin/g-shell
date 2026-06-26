#ifndef COMMAND_H
#define COMMAND_H

#define MAX_ARGS 64
#include <stdbool.h>

typedef enum {
    NO_OP,
    AND_OP,
    PIPE_OP,
} LogicalOp;

typedef struct Command {
    char *args[MAX_ARGS];
    int arg_count;
    char *cmd;

    // I/O Redirection state
    int fd_out;
    int fd_err;
    int saved_stdout;
    int saved_stderr;
    bool background;

    LogicalOp op;
    // next command to run
    struct Command *next;
} Command;
#endif // COMMAND_H
