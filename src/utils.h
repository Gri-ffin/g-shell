#ifndef CODECRAFTERS_SHELL_C_UTILS_H
#define CODECRAFTERS_SHELL_C_UTILS_H

int compare(const void *a, const void *b);

char **resolve_files_in_dir(int *count, const char *dirname);
#endif //CODECRAFTERS_SHELL_C_UTILS_H
