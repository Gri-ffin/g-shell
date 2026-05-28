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

      if (strcmp(command, "xyz") == 0) {
        printf("%s: command not found\n", command);
      }
    }
  }

  return 0;
}
