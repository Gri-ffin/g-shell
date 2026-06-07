#ifndef BUILTINS_H
#define BUILTINS_H
extern char *builtins[];

void handle_type(char *arg, int fd);

void handle_echo(char **args, int fd);

void run_program(const char *program, char **args, int fd);

void handle_pwd(int fd);

void handle_cd(const char *path);
#endif
