#ifndef PATH_H
#define PATH_H
#include <stdbool.h>
#include <stddef.h>

bool resolve_path(const char *cmd, char *out_path, size_t max_len);
#endif // PATH_H
