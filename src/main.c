#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/readline.h>

#include "builtins/auto_completion/autocompletion.h"
#include "builtins/builtins.h"
#include "builtins/jobs.h"
#include "command.h"
#include "parser.h"
#include "path.h"

// THESE SHOULD NEVER GO TO THE BACKGROUND JOB
static const char *const no_background[] = {"go", "exit", "cd", NULL};

/**
 *
 * @param cmd command name
 * @return if the command can be run in the background or not
 */
static bool can_background(const char *cmd) {
  for (int i = 0; no_background[i]; i++)
    if (strcmp(cmd, no_background[i]) == 0)
      return false;
  return true;
}

int main() {
  int code = setvbuf(stdout, NULL, _IONBF, 0);
  if (code != 0) {
    perror("setvbuf");
  }
  printf("\033[H\033[2J");
  char *input = NULL;

  rl_attempted_completion_function = shell_completion;

  while (true) {
    jobs_reap();
    input = readline("$ ");
    fflush(stdout);

    if (input == NULL) {
      break;
    }

    Command cmd = parse_command(input);
    if (cmd.arg_count == 0) {
      free(input);
      continue;
    }

    // guard against overflow
    if (cmd.arg_count >= MAX_ARGS - 1) {
      fprintf(stderr, "too many arguments\n");
      free(input);
      continue;
    }

    Command *current_command = &cmd;
    int last_status = 0;

    // track the read-end of a pipe from the previous command in the chain
    int prev_pipe_read = -1;

    while (current_command != NULL) {
      int pipefd[2] = {-1, -1};
      int saved_stdin = -1;

      // If this command pipes to the next, create a pipe
      if (current_command->op == PIPE_OP) {
        if (pipe(pipefd) == -1) {
          perror("pipe");
          break;
        }
        // Redirect stdout to the pipe write-end, unless the user explicitly
        // redirected it
        if (current_command->fd_out == STDOUT_FILENO) {
          // the current commands write to the pipe's stdin
          current_command->fd_out = pipefd[1];
        } else {
          // If fd_out was redirected to a file (e.g., cmd > file | cmd2),
          // close the write end immediately so the next command gets an EOF.
          close(pipefd[1]);
        }
      }

      // Map the previous pipe's read-end to STDIN
      if (prev_pipe_read != -1) {
        // save actual stdin
        saved_stdin = dup(STDIN_FILENO);
        // map the output from the previous pipe to the actual stdin input of
        // cur command
        dup2(prev_pipe_read, STDIN_FILENO);
        close(prev_pipe_read);
      }

      if (current_command->fd_out != STDOUT_FILENO) {
        current_command->saved_stdout = dup(STDOUT_FILENO);
        dup2(current_command->fd_out, STDOUT_FILENO);
      }

      if (current_command->fd_err != STDERR_FILENO) {
        current_command->saved_stderr = dup(STDERR_FILENO);
        dup2(current_command->fd_err, STDERR_FILENO);
      }

      if (strcmp(current_command->cmd, "exit") == 0) {
        free(input);
        exit(0);
      }
      const builtin_fn fn = find_builtin(current_command->cmd);
      if (current_command->background && can_background(current_command->cmd)) {
        const pid_t pid = fork();
        if (pid == 0) {
          if (fn) {
            fn(current_command);
            exit(0);
          }
          run_program(current_command->cmd, current_command->args);
          exit(0);
        }
        jobs_add(pid, current_command->cmd);
        last_status = 0;
      } else {
        if (fn)
          last_status = fn(current_command);
        else
          last_status =
              run_program(current_command->cmd, current_command->args);
      }

      if (current_command->saved_stdout != -1) {
        dup2(current_command->saved_stdout,
             STDOUT_FILENO);                  // Put the terminal back
        close(current_command->saved_stdout); // Close the duplicate
        close(
            current_command
                ->fd_out); // NOTE: This safely closes pipefd[1] in the parent!
      }

      // Restore standard error if we altered it
      if (current_command->saved_stderr != -1) {
        dup2(current_command->saved_stderr,
             STDERR_FILENO);                  // Put the terminal back
        close(current_command->saved_stderr); // Close the duplicate
        close(current_command->fd_err);       // Close the file we wrote to
      }

      if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
      }

      if (current_command->op == AND_OP && last_status != 0) {
        if (pipefd[0] != -1)
          close(pipefd[0]); // Don't leak the FD on break
        break;
      }
      if ((current_command->op == AND_OP || current_command->op == PIPE_OP) &&
          current_command->next->arg_count == 0) {
        fprintf(stderr, "the chaining command wasn't given.\n");
        if (pipefd[0] != -1)
          close(pipefd[0]); // Don't leak the FD on break
        break;
      }

      // Carry the read-end of the pipe over to the next loop iteration
      prev_pipe_read = pipefd[0];

      current_command = current_command->next;
    }
    // Free the dynamically allocated parts of the command chain
    // We skip the head node (&cmd) because it's allocated on the stack!
    Command *node_to_free = cmd.next;
    while (node_to_free != NULL) {
      Command *tmp = node_to_free->next;
      free(node_to_free);
      node_to_free = tmp;
    }

    free(input);
  }

  return 0;
}
