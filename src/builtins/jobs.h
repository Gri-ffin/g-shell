#ifndef CODECRAFTERS_SHELL_C_JOBS_H
#define CODECRAFTERS_SHELL_C_JOBS_H
#include <sys/types.h>

#define MAX_JOBS 64

typedef enum { JOB_RUNNING, JOB_DONE } JobStatus;

typedef struct {
    char *cmd;
    int id;
    pid_t pid;
    JobStatus status;
} Job;

void jobs_add(pid_t pid, const char *cmd);

void jobs_print();

void jobs_reap();

#endif //CODECRAFTERS_SHELL_C_JOBS_H
