#include <stdio.h>
#include <string.h>
#include "complete.h"
#include <stdlib.h>

static CompleteCommand *complete_commands = NULL;
static int complete_commands_count = 0;
static int capacity = 0;

void register_complete(const char *path, const char *program) {
    for (int i = 0; i < complete_commands_count; i++) {
        if (strcmp(complete_commands[i].program, program) == 0) {
            free(complete_commands[i].program);
            complete_commands[i].program = strdup(program);
            return;
        }
    }

    if (complete_commands_count == capacity) {
        capacity = capacity == 0 ? 8 : capacity * 2;
        CompleteCommand *tmp = realloc(complete_commands, capacity * sizeof(CompleteCommand));
        if (tmp == NULL) {
            // don't destroy the global state.
            fprintf(stderr, "error: out of memory. Could not register autocomplete for '%s'.\n", program);
            return;
        }
        complete_commands = tmp;
    }

    complete_commands[complete_commands_count++] = (CompleteCommand){
        .program = strdup(program),
        .path = strdup(path),
    };
}

/**
 * @param args_count the count of the arguments passed
 * @param args the command line arguments
 */
void handle_complete(char **args, const int args_count) {
    if (args_count <= 1) {
        fprintf(stderr, "error: complete should have at least arguments.\n");
        return;
    }
    const char *arg = args[1];

    if (strcmp(arg, "-p") == 0) {
        if (args_count < 3) {
            fprintf(stderr, "error: program name should be specified.\n");
            return;
        }
        const char *command = args[2];
        for (int i = 0; i < complete_commands_count; i++) {
            if (strcmp(complete_commands[i].program, command) == 0) {
                const char *program = complete_commands[i].program;
                const char *path = complete_commands[i].path;
                printf("%s completion function is in '%s'\n", program, path);
                return;
            }
        }
        fprintf(stderr, "complete: %s: can't find the completion specification.\n", command);
    } else if (strcmp(arg, "-R") == 0) {
        if (args_count < 4) {
            fprintf(stderr, "error: program name and path should be specified.\n");
            return;
        }
        const char *path = args[2];
        const char *program = args[3];
        register_complete(path, program);
    } else {
        fprintf(stderr, "%s: invalid argument.\n", arg);
    }
}
