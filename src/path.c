#include "path.h"
#include "utils.h"
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_DELIMITER ":"
#define INITIAL_CAPACITY 10

/**
 * @brief Searches directories in the PATH environment variable to find a
 * matching executable.
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
 * @brief Scans all PATH directories to build a sorted, deduplicated list of
 * available executables.
 * @param count Pointer to an integer where the total number of unique
 * executables will be stored.
 * @return A null-terminated array of dynamically allocated strings, or NULL on
 * error.
 */
char **resolve_executables_in_path(int *count) {
  DynamicArray *executables_array = create_dynamic_array();
  if (!executables_array) {
    return NULL;
  }
  const char *env_path = getenv("PATH");
  if (env_path == NULL) {
    free(executables_array);
    return NULL;
  }

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
            char *cmd_name = strdup(entry->d_name);
            if (!array_push(cmd_name, executables_array)) {
              // free everything
              free(cmd_name);
              closedir(dir);
              free(path_copy);
              array_free(executables_array);
              return NULL;
            }
          }
        }
      }
      closedir(dir);
    }
    token = strtok(NULL, PATH_DELIMITER);
  }
  free(path_copy);

  // If we found nothing, clean up and return
  if (executables_array->count == 0) {
    *count = 0;
    array_free(executables_array);
    return NULL;
  }

  // Now clean the duplicates
  // we need to cast to char** instead of the current void**
  char **executables = (char **)executables_array->items;
  // initiliaze count too since we will need it
  *count = executables_array->count;
  qsort((void *)executables, *count, sizeof(char *), compare);
  int unique = 1;

  for (int i = 1; i < *count; i++) {
    if (strcmp(executables[i], executables[i - 1]) == 0) {
      free(executables[i]); // duplicate
    } else {
      executables[unique++] = executables[i];
    }
  }

  *count = unique;
  executables_array->count = unique;

  // sentinel
  array_push(NULL, executables_array);

  char **final_array = (char **)executables_array->items;
  // Just 'free', not 'array_free'! We want to keep the strings.
  free(executables_array);

  return final_array;
}
