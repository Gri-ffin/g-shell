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

char **resolve_executables_in_path(int *count) {
    char **executables = NULL;
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
                        char **tmp = realloc(executables, sizeof(char *) * (*count + 1));
                        // guard against realloc failing
                        if (tmp == NULL) {
                            free(executables);
                            return NULL;
                        }
                        executables = tmp;
                        // check for duplicates first, skip if any
                        bool is_duplicate_exists = false;
                        for (int i = 0; i < *count; i++) {
                            if (strcmp(executables[i], entry->d_name) == 0) {
                                is_duplicate_exists = true;
                                break;
                            }
                        }
                        // if no duplicates, get only the program name
                        if (!is_duplicate_exists) {
                            executables[*count] = strdup(entry->d_name);
                            (*count)++;
                        }
                    }
                }
            }
            closedir(dir);
        }
        token = strtok(NULL, PATH_DELIMITER);
    }
    free(path_copy);
    return executables;
}
