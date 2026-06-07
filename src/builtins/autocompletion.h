#ifndef AUTOCOMPLETION_H
#define AUTOCOMPLETION_H

char *builtin_generator(const char *text, int state);

char **shell_completion(const char *text, int start, int end);
#endif // AUTOCOMPLETION_H
