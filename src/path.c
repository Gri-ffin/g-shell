#include "path.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define PATH_DELIMITER ":"
#define INITIAL_CAPACITY 10

/**
 * @brief Comparison helper function for qsort to sort string arrays alphabetically.
 */
int compare(const void *a, const void *b) {
    const char *str_a = *(const char **) a;
    const char *str_b = *(const char **) b;
    return strcmp(str_a, str_b);
}

/**
 * @brief Searches directories in the PATH environment variable to find a matching executable.
 * @param cmd      The name of the command to look for.
 * @param out_path Buffer to store the absolute path if found.
 * @param max_len  The maximum capacity of the out_path buffer.
 * @return true if the command is found and executable, false otherwise.
 */
bool resolve_path(const char *cmd, char *out_path, size_t max_len) {
    const char *env_path = getenv("PATH");
    if (env_path == NULL) {
        return false;
    }

    char *path_copy = strdup(env_path);
    char *token = strtok(path_copy, PATH_DELIMITER);

    while (token != NULL) {
        snprintf(out_path, max_len, "%s/%s", token, cmd);

        if (access(out_path, X_OK) == 0) {
            free(path_copy);
            return true;
        }

        token = strtok(NULL, PATH_DELIMITER);
    }

    free(path_copy);
    return false;
}

/**
 * @brief Scans all PATH directories to build a sorted, deduplicated list of available executables.
 * @param count Pointer to an integer where the total number of unique executables will be stored.
 * @return A null-terminated array of dynamically allocated strings, or NULL on error.
 */
char **resolve_executables_in_path(int *count) {
    int capacity = INITIAL_CAPACITY;
    char **executables = malloc(capacity * sizeof(char *));
    if (!executables) return NULL;
    const char *env_path = getenv("PATH");
    if (env_path == NULL)
        return NULL;

    char *path_copy = strdup(env_path);
    char *token = strtok(path_copy, PATH_DELIMITER);

    while (token != NULL) {
        DIR *dir = opendir(token);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                char full_path[PATH_MAX];
                // build the path
                snprintf(full_path, PATH_MAX, "%s/%s", token, entry->d_name);

                // check if its executable
                if (access(full_path, X_OK) == 0) {
                    struct stat st;
                    stat(full_path, &st);
                    if (S_ISREG(st.st_mode)) {
                        if (*count >= capacity) {
                            capacity *= 2;
                            char **tmp = realloc(executables, sizeof(char *) * capacity);
                            // guard against realloc failing
                            if (tmp == NULL) {
                                for (int i = 0; i < *count; i++) {
                                    free(executables[i]);
                                }
                                free(executables);
                                return NULL;
                            }
                            executables = tmp;
                        }
                        // naively add duplicates for now, we will remove them later
                        // if we try to check duplicates its O(N^2)
                        executables[*count] = strdup(entry->d_name);
                        (*count)++;
                    }
                }
            }
            closedir(dir);
        }
        token = strtok(NULL, PATH_DELIMITER);
    }
    free(path_copy);

    // Now clean the duplicates
    qsort(executables, *count, sizeof(char *), compare);
    int unique = 1;

    for (int i = 1; i < *count; i++) {
        if (strcmp(executables[i], executables[i - 1]) == 0) {
            free(executables[i]); // duplicate
        } else {
            executables[unique++] = executables[i];
        }
    }

    *count = unique;
    executables[*count] = NULL;

    return executables;
}
