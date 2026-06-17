#ifndef CODECRAFTERS_SHELL_C_UTILS_H
#define CODECRAFTERS_SHELL_C_UTILS_H
#include <stdbool.h>

typedef struct {
    void **items;
    int count;
    int capacity;
} DynamicArray;

DynamicArray *create_dynamic_array(void);

bool array_push(void *item, DynamicArray *array);

bool array_pop(DynamicArray *array);

bool array_free(DynamicArray *array);

bool array_remove_item(DynamicArray *array, const void *item);

int compare(const void *a, const void *b);

char **resolve_files_or_dirs_in_dir(int *count, const char *dirname);
#endif //CODECRAFTERS_SHELL_C_UTILS_H
