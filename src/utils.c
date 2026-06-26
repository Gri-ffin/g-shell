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
 * @return a newly allocated dynamic array, NULL on error
 */
DynamicArray *create_dynamic_array() {
  DynamicArray *dyn_array = malloc(sizeof(DynamicArray));
  if (!dyn_array) {
    fprintf(stderr, "error: failed to allocate mem for dynamic array.\n");
    return NULL;
  }

  dyn_array->capacity = INITIAL_CAPACITY;
  dyn_array->count = 0;
  dyn_array->items = (void **)malloc(dyn_array->capacity * sizeof(void *));

  if (!dyn_array->items) {
    fprintf(stderr, "error: failed to allocate mem for dynamic array items.\n");
    free((void *)dyn_array->items);
    free(dyn_array);
    return NULL;
  }

  return dyn_array;
}

/**
 *
 * @param item item to push to the end of the array
 * @param array the array which will hold the item
 * @return true if operation succeed, otherwise false
 */
bool array_push(void *item, DynamicArray *array) {
  if (array->count >= array->capacity) {
    const int new_capacity = array->capacity * 2;
    void **tmp =
        (void **)realloc((void *)array->items, new_capacity * sizeof(void *));
    if (!tmp) {
      fprintf(stderr, "error: failed to allocate mem for dynamic array.\n");
      return false;
    }

    array->items = tmp;
    array->capacity = new_capacity;
  }

  array->items[array->count++] = item;
  return true;
}

/**
 *
 * @param array array to pop the last item from
 * @return true if operation succeed, otherwise false
 */
bool array_pop(DynamicArray *array) {
  if (!array) {
    return false;
  }

  if (array->count <= 0) {
    fprintf(stderr, "error: array is empty.\n");
    return false;
  }
  array->count--;
  free(array->items[array->count]);
  return true;
}

/**
 *
 * @param array array to free
 * @return true on success, false otherwise
 */
bool array_free(DynamicArray *array) {
  if (!array) {
    return false;
  }

  for (int i = 0; i < array->count; i++) {
    free(array->items[i]);
  }
  free((void *)array->items);
  free(array);
  return true;
}

/**
 *
 * @param array array to delete the item from
 * @param item the item to delete
 * @return true if success, false otherwise
 */
bool array_remove_item(DynamicArray *array, const void *item) {
  if (!array)
    return false;

  for (int i = 0; i < array->count; i++) {
    if (array->items[i] == item) {
      // Shift everything down to overwrite the removed item
      memmove((void *)&array->items[i], (void *)&array->items[i + 1],
              (array->count - i - 1) * sizeof(void *));

      array->count--;
      return true; // Successfully found and removed
    }
  }

  // item not found
  return false;
}

/**
 * @brief Comparison helper function for qsort to sort string arrays
 * alphabetically.
 */
int compare(const void *a, const void *b) {
  const char *str_a = *(const char **)a;
  const char *str_b = *(const char **)b;
  return strcmp(str_a, str_b);
}

/**
 *
 * @param count the count of files and dirs that should be updated
 * @param dirname the directory name to scan for files and dirs
 * @return either NULL or the names of the files/dirs in the directory
 */
char **resolve_files_or_dirs_in_dir(int *count, const char *dirname) {
  DIR *dir = opendir(dirname);
  DynamicArray *filenames_array = create_dynamic_array();
  if (!filenames_array)
    return NULL;
  if (!dir) {
    array_free(filenames_array);
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
    if (stat(full_path, &st) == 0 &&
        (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))) {
      size_t file_len = strlen(dirname) + strlen(entry->d_name) + 2;
      char *filename = malloc(file_len);
      snprintf(filename, file_len, "%s/%s", dirname, entry->d_name);
      if (!array_push(filename, filenames_array)) {
        free(filename);
      }
    }
  }
  closedir(dir);
  *count = filenames_array->count;
  char **filenames = (char **)filenames_array->items;
  free(filenames_array);
  return filenames;
}
