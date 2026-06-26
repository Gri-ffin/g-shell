#ifndef BUILTINS_H
#define BUILTINS_H
#include "../command.h"

extern char *builtins[];
extern const int builtins_count;

int handle_type(char *arg);

int handle_echo(char **args);

int run_program(const char *program, char **args);

int handle_pwd();

void handle_cd(const char *path);

void handle_jobs();


typedef int (*builtin_fn)(Command *cmd);

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
