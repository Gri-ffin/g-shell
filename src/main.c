#include "builtins.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 64

int parse_input(char *input, char **args) {
  bool inside_single_quotes = false;
  bool inside_double_quotes = false;
  char *start_arg = NULL;
  char *ptr = input;
  int arg_count = 0;

  while (*ptr != '\0') {
    if (*ptr == '\'') {
      inside_single_quotes = !inside_single_quotes;
      memmove(ptr, ptr + 1, strlen(ptr));
      if (!start_arg) {
        start_arg = ptr;
      }
      continue;
    } else if (*ptr == '"') {
      inside_double_quotes = !inside_double_quotes;
      memmove(ptr, ptr + 1, strlen(ptr));
      if (!start_arg) {
        start_arg = ptr;
      }
      continue;
    }

    if (*ptr == ' ' && !inside_single_quotes && !inside_double_quotes) {
      if (start_arg) {
        *ptr = '\0';
        args[arg_count++] = start_arg;
        start_arg = NULL;
      }
    } else {
      if (!start_arg) {
        start_arg = ptr;
      }
    }
    ptr++;
  }

  if (inside_double_quotes || inside_single_quotes) {
    fprintf(stderr, "syntax error: unterminated quote\n");
    return 0;
  }

  if (start_arg) {
    args[arg_count++] = start_arg;
  }
  args[arg_count] = NULL;
  return arg_count;
}

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

    int arg_count = parse_input(input, args);
    if (arg_count == 0)
      continue;

    char *cmd = args[0];

    if (strcmp(cmd, "exit") == 0) {
      exit(0);
    } else if (strcmp(cmd, "echo") == 0) {
      handle_echo(args);
    } else if (strcmp(cmd, "type") == 0) {
      handle_type(args[1]);
    } else if (strcmp(cmd, "pwd") == 0) {
      handle_pwd();
    } else if (strcmp(cmd, "cd") == 0) {
      handle_cd(args[1]);
    } else {
      run_program(cmd, args);
    }
  }

  free(input);
  return 0;
}
