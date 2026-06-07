#ifndef PARSER_H
#define PARSER_H

int parse_input(char *input, char **args);

int check_and_handle_redirection(char **args, int *target_stream);

int parse_redirection(const char *arg, int *target_stream, const char *filename);
#endif // PARSER_H
