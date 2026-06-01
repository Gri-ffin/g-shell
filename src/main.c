#include "builtins.h"
#include "parser.h"
#include "path.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

    int arg_count = parse_input(input, args);
    if (arg_count == 0)
      continue;

    char *cmd = args[0];
    int fd = check_and_handle_redirection(args);

    if (strcmp(cmd, "exit") == 0) {
      exit(0);
    } else if (strcmp(cmd, "echo") == 0) {
      handle_echo(args, fd);
    } else if (strcmp(cmd, "type") == 0) {
      handle_type(args[1]);
    } else if (strcmp(cmd, "pwd") == 0) {
      handle_pwd();
    } else if (strcmp(cmd, "cd") == 0) {
      handle_cd(args[1]);
    } else {
      run_program(cmd, args);
    }
    if (fd != 1)
      close(fd);
  }

  free(input);
  return 0;
}
