#include "path.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
