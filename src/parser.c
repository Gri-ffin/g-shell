#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define STDOUT_REDIRECTION_SHORT ">"

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

int check_and_handle_redirection(char **args) {
  for (int i = 1; args[i] != NULL; i++) {
    if (strcmp(args[i], STDOUT_REDIRECTION_SHORT) == 0) {
      char *filename = args[i + 1];
      if (filename == NULL) {
        fprintf(stderr, "Syntax error: expected file after '>'\n");
        return -1;
      }

      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        perror("open");
        return -1;
      }

      // shift arguments to remove the redirection operator and the filename
      int j = i;
      while (args[j + 2] != NULL) {
        args[j] = args[j + 2];
        j++;
      }
      args[j] = NULL;

      return fd;
    }
  }

  return 1;
}
