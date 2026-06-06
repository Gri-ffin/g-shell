int parse_input(char *input, char **args);
int check_and_handle_redirection(char **args, int *target_fd);
int parse_redirection(const char *arg, int *target_fd, const char *filename);
