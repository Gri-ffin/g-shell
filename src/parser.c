#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STDOUT_REDIRECTION_SHORT ">"
#define STDOUT_REDIRECTION "1>"
#define STDERR_REDIRECTION "2>"

int parse_input(char *input, char **args) {
  bool inside_single_quotes = false;
  bool inside_double_quotes = false;
  char *start_arg = NULL;
  char *ptr = input;
  int arg_count = 0;

  while (*ptr != '\0') {
    if (*ptr == '\'' && !inside_double_quotes) {
      inside_single_quotes = !inside_single_quotes;
      memmove(ptr, ptr + 1, strlen(ptr));
      if (!start_arg) {
        start_arg = ptr;
      }
      continue;
    } else if (*ptr == '"' && !inside_single_quotes) {
      inside_double_quotes = !inside_double_quotes;
      memmove(ptr, ptr + 1, strlen(ptr));
      if (!start_arg) {
        start_arg = ptr;
      }
      continue;
    } else if (*ptr == '\\' && !inside_double_quotes && !inside_single_quotes) {
      memmove(ptr, ptr + 1, strlen(ptr));
      if (*ptr != '\0') {
        if (!start_arg) {
          start_arg = ptr;
        }
        ptr++;
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

int parse_redirection(char *arg, int *target_fd, char *filename) {
  *target_fd = STDOUT_FILENO;

  bool is_stdout = (strcmp(arg, STDOUT_REDIRECTION) == 0 ||
                    strcmp(arg, STDOUT_REDIRECTION_SHORT) == 0);
  bool is_stderr = (strcmp(arg, STDERR_REDIRECTION) == 0);

  if (is_stdout || is_stderr) {
    if (is_stdout)
      *target_fd = STDOUT_FILENO;
    else if (is_stderr)
      *target_fd = STDERR_FILENO;

    if (filename == NULL) {
      fprintf(stderr, "Shell error: No syntax file specified\n");
      return -1;
    }

    int fd_file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_file == -1)
      perror(filename);
    return fd_file;
  }

  return -1;
}

int check_and_handle_redirection(char **args, int *target_fd) {
  for (int i = 1; args[i] != NULL; i++) {
    int fd = parse_redirection(args[i], target_fd, args[i + 1]);
    if (fd != -1) {
      int j = i;
      while (args[j + 2] != NULL) {
        args[j] = args[j + 2];
        j++;
      }
      args[j] = NULL;
      return fd;
    }
  }

  return STDOUT_FILENO;
}
