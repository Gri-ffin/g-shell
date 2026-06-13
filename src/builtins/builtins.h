#ifndef BUILTINS_H
#define BUILTINS_H
#include "../command.h"

extern char *builtins[];
extern const int builtins_count;

void handle_type(char *arg, int fd);

void handle_echo(char **args, int fd);

void run_program(const char *program, char **args, int fd);

void handle_pwd(int fd);

void handle_cd(const char *path);

void handle_complete(char **args, int args_count);

typedef void (*builtin_fn)(Command *cmd);

typedef struct {
    const char *name;
    builtin_fn fn;
} BuiltinEntry;

/**
 * @param name the command name which to find the handler for
 * @return the handler for the command name
 */
builtin_fn find_builtin(const char *name);
#endif
