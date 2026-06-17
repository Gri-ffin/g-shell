#ifndef CODECRAFTERS_SHELL_C_COMPLETE_H
#define CODECRAFTERS_SHELL_C_COMPLETE_H
#include "../utils.h"

typedef struct {
    char *path;
    char *program;
} CompleteCommand;

DynamicArray *get_completion_scripts();

void register_complete(const char *path, const char *program);

void handle_complete(char **args, int args_count);

void remove_complete(const CompleteCommand *command);
#endif //CODECRAFTERS_SHELL_C_COMPLETE_H
