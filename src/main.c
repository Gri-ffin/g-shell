#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while (TRUE) {
    printf("$ ");
    char command[1024];
    if (fgets(command, sizeof(command), stdin) != NULL) {

      command[strcspn(command, "\n")] = '\0';

      if (strcmp(command, "exit") == 0) {
        exit(0);
      } else if (strcmp(command, "echo") == 0) {
        printf("\n");
      } else if (strncmp(command, "echo ", 5) == 0) {
        printf("%s\n", command + 5);
      } else if (strncmp(command, "type ", 5) == 0) {
        if (strcmp(command + 5, "echo") == 0 ||
            strcmp(command + 5, "exit") == 0) {
          printf("%s is a builtin command.\n", command + 5);
        } else {
          printf("%s: not found\n", command + 5);
        }
      } else {
        printf("%s: command not found\n", command);
      }
    }
  }

  return 0;
}
