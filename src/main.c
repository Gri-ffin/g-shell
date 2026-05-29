#include "builtins.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  setbuf(stdout, NULL);
  char *input = NULL;
  size_t cap = 0;
  ssize_t len;

  while (true) {
    printf("$ ");

    len = getline(&input, &cap, stdin);
    if (len == -1) {
      printf("Exiting Shell...\n");
      break;
    }

    input[strcspn(input, "\n")] = '\0';

    char *cmd = strtok(input, " ");
    if (cmd == NULL)
      continue; // User just pressed Enter, skip

    char *arg = strtok(NULL, ""); // Get EVERYTHING left after the first space
    if (arg != NULL) {
      arg += strspn(arg, " ");
      if (*arg == '\0') {
        arg = NULL;
      }
    }

    if (strcmp(cmd, "exit") == 0) {
      exit(0);
    } else if (strcmp(cmd, "echo") == 0) {
      handle_echo(arg);
    } else if (strcmp(cmd, "type") == 0) {
      handle_type(arg);
    } else {
      printf("%s: command not found\n", cmd);
    }
  }

  free(input);
  return 0;
}
