#include "builtins.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 64

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

    char *args[MAX_ARGS];
    int arg_count = 0;

    char *token = strtok(input, " ");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
      args[arg_count++] = token;
      token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; // null terminate the args

    if (arg_count == 0) {
      continue;
    }

    char *cmd = args[0];

    if (strcmp(cmd, "exit") == 0) {
      exit(0);
    } else if (strcmp(cmd, "echo") == 0) {
      handle_echo(args);
    } else if (strcmp(cmd, "type") == 0) {
      handle_type(args[1]);
    } else {
      run_program(cmd, args);
    }
  }

  free(input);
  return 0;
}
