#include "utils.h"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define INITIAL_CAPACITY 10

/**
 *
 * @param count the count of files that should be updated
 * @param dirname the directory name to scan for files
 * @return either NULL or the names of the files in the directory
 */
char **resolve_files_in_dir(int *count, const char *dirname) {
    DIR *dir = opendir(dirname);
    int capacity = INITIAL_CAPACITY;
    char **filenames = malloc(capacity * sizeof(char *));
    if (!filenames) return NULL;
    if (!dir) {
        free(filenames);
        return NULL;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[strlen(dirname) + strlen(entry->d_name) + 2];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirname, entry->d_name);

        struct stat st;
        stat(full_path, &st);
        if (S_ISREG(st.st_mode)) {
            if (*count >= capacity) {
                capacity *= 2;
                char **tmp = realloc(filenames, capacity * sizeof(char *));
                if (tmp == NULL) {
                    for (int i = 0; i < *count; i++) {
                        free(filenames[i]);
                    }
                    free(filenames);
                    closedir(dir);
                    return NULL;
                }
                filenames = tmp;
            }
            filenames[*count] = strdup(entry->d_name);
            (*count)++;
        }
    }
    closedir(dir);
    return filenames;
}
