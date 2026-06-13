#ifndef CODECRAFTERS_SHELL_C_COMPLETE_H
#define CODECRAFTERS_SHELL_C_COMPLETE_H

typedef struct {
    char *path;
    char *program;
} CompleteCommand;

void register_complete(const char *path, const char *program);

void handle_complete(char **args, int args_count);
#endif //CODECRAFTERS_SHELL_C_COMPLETE_H
