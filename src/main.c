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

    // guard against overflow
    if (arg_count >= MAX_ARGS - 1) {
      fprintf(stderr, "too many arguments\n");
      continue;
    }

    char *cmd = args[0];
    int target_fd = STDOUT_FILENO;
    int fd = check_and_handle_redirection(args, &target_fd);

    if (fd == -1)
      continue;

    int saved_stderr = -1;
    // don't overwrite the program main stderr
    if (target_fd == STDERR_FILENO) {
      saved_stderr = dup(STDERR_FILENO);
      dup2(fd, STDERR_FILENO);
    }

    if (strcmp(cmd, "exit") == 0) {
      exit(0);
    } else if (strcmp(cmd, "echo") == 0) {
      handle_echo(args, fd);
    } else if (strcmp(cmd, "type") == 0) {
      handle_type(args[1], fd);
    } else if (strcmp(cmd, "pwd") == 0) {
      handle_pwd(fd);
    } else if (strcmp(cmd, "cd") == 0) {
      handle_cd(args[1]);
    } else {
      run_program(cmd, args, fd);
    }

    // restore the main stderr
    if (saved_stderr != -1) {
      dup2(saved_stderr, STDERR_FILENO);
      close(saved_stderr);
    }
    if (fd != STDOUT_FILENO)
      close(fd);
  }

  free(input);
  return 0;
}
