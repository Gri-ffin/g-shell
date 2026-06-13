#ifndef BUILTINS_H
#define BUILTINS_H
#include "../command.h"

extern char *builtins[];
extern const int builtins_count;

void handle_type(char *arg);

void handle_echo(char **args);

void run_program(const char *program, char **args);

void handle_pwd();

void handle_cd(const char *path);


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
