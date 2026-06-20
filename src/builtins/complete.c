#include <stdio.h>
#include <string.h>
#include "complete.h"
#include <stdlib.h>
#include "../utils.h"

static DynamicArray *complete_commands_array = NULL;

/**
 *
 * @return a pointer to the array containing the completion commands
 */
DynamicArray *get_completion_scripts() {
    if (!complete_commands_array) return NULL;
    return complete_commands_array;
}

/**
 *
 * @param path path to the completion script
 * @param program the name of the program
 */
int register_complete(const char *path, const char *program) {
    if (!complete_commands_array) {
        complete_commands_array = create_dynamic_array();
    }
    const int count = complete_commands_array->count;
    CompleteCommand **items = (CompleteCommand **) complete_commands_array->items;
    for (int i = 0; i < count; i++) {
        if (strcmp(items[i]->program, program) == 0) {
            free(items[i]->path);
            items[i]->path = strdup(path);
            return EXIT_SUCCESS;
        }
    }

    CompleteCommand *new_complete_command = malloc(sizeof(CompleteCommand));
    if (!new_complete_command) {
        fprintf(stderr, "error: out of memory.\n");
        return EXIT_FAILURE;
    }

    new_complete_command->program = strdup(program);
    new_complete_command->path = strdup(path);

    if (!array_push(new_complete_command, complete_commands_array)) {
        free(new_complete_command->program);
        free(new_complete_command->path);
        free(new_complete_command);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 *
 * @param command command to remove
 */
int remove_complete(const CompleteCommand *command) {
    if (!complete_commands_array) {
        fprintf(stderr, "error: no program completion has been registered yet.\n");
        return EXIT_FAILURE;
    }

    if (!array_remove_item(complete_commands_array, command)) {
        fprintf(stderr, "error: such program isn't currently registered.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @param args_count the count of the arguments passed
 * @param args the command line arguments
 */
int handle_complete(char **args, const int args_count) {
    if (args_count <= 1) {
        fprintf(stderr, "error: complete should have at least arguments.\n");
        return EXIT_FAILURE;
    }
    const char *arg = args[1];

    if (strcmp(arg, "-p") == 0) {
        if (args_count < 3) {
            fprintf(stderr, "error: program name should be specified.\n");
            return EXIT_FAILURE;
        }

        const char *command = args[2];
        // does the array exists at all?
        if (!complete_commands_array || complete_commands_array->count == 0) {
            fprintf(stderr, "complete: %s: can't find the completion specification.\n", command);
            return EXIT_FAILURE;
        }
        const CompleteCommand **items = (const CompleteCommand **) complete_commands_array->items;

        for (int i = 0; i < complete_commands_array->count; i++) {
            if (strcmp(items[i]->program, command) == 0) {
                const char *program = items[i]->program;
                const char *path = items[i]->path;
                printf("%s completion function is in '%s'\n", program, path);
                return EXIT_SUCCESS;
            }
        }
        fprintf(stderr, "complete: %s: can't find the completion specification.\n", command);
        return EXIT_FAILURE;
    }
    if (strcmp(arg, "-R") == 0) {
        if (args_count < 4) {
            fprintf(stderr, "error: program name and path should be specified.\n");
            return EXIT_FAILURE;
        }
        const char *path = args[2];
        const char *program = args[3];
        return register_complete(path, program);
    }
    if (strcmp(arg, "-d") == 0) {
        if (args_count < 3) {
            fprintf(stderr, "error: program name should be specified.\n");
            return EXIT_FAILURE;
        }

        if (!complete_commands_array || complete_commands_array->count == 0) {
            fprintf(stderr, "complete: no complete scripts are registered yet.\n");
            return EXIT_FAILURE;
        }

        const CompleteCommand **items = (const CompleteCommand **) complete_commands_array->items;
        const CompleteCommand *command = NULL;
        const char *program = args[2];
        for (int i = 0; i < complete_commands_array->count; i++) {
            if (strcmp(items[i]->program, program) == 0) {
                command = (CompleteCommand *) items[i];
                return remove_complete(command);
            }
        }

        fprintf(stderr, "complete: %s: can't find the program.\n", program);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "%s: invalid argument.\n", arg);
    return EXIT_FAILURE;
}
