#include "builtins.h"
#include "path.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void handle_echo(char **args) {
  for (int i = 1; args[i] != NULL; i++) {
    printf("%s", args[i]);
    if (args[i + 1] != NULL) {
      printf(" ");
    }
  }
  printf("\n");
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

void handle_pwd() {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("pwd failed\n");
  }
}

void handle_cd(const char *path) {
  if (path == NULL) {
    printf("cd: missing argument\n");
    return;
  }

  switch (path[0]) {
  case '/':
    if (chdir(path) == -1) {
      printf("cd: %s: %s\n", path, strerror(errno));
      return;
    }
    return;
  default:
    printf("Invalid directory %s\n", path);
    return;
  }
}

void run_program(const char *program, char **args) {
  char resolved_path[PATH_MAX];
  if (resolve_path(program, resolved_path, sizeof(resolved_path))) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("failed to create the fork");
      return;
    }

    if (pid == 0) {
      execv(resolved_path, args);
      perror("execv failed");
      exit(1);
    } else {
      int status;
      waitpid(pid, &status, 0);
    }
  } else {
    printf("%s: command not found\n", program);
  }
}
