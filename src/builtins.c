#include "builtins.h"
#include "externals.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

void handle_echo(char *arg) {
  if (arg == NULL) {
    printf("\n");
  } else {
    printf("%s\n", arg);
  }
}

void handle_type(char *arg) {
  if (arg == NULL) {
    printf("type: missing argument\n");
    return;
  }

  if (strcmp(arg, "echo") == 0 || strcmp(arg, "exit") == 0 ||
      strcmp(arg, "type") == 0) {
    printf("%s is a shell builtin\n", arg);
    return;
  }

  char resolved_path[PATH_MAX];
  if (resolve_path(arg, resolved_path, sizeof(resolved_path))) {
    printf("%s is %s\n", arg, resolved_path);
  } else {
    printf("%s: not found\n", arg);
  }
}
