#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

static Job jobs_table[MAX_JOBS];
static int num_jobs = 0;
static int next_id = 1;

/**
 *
 * @param pid process pid
 * @param cmd name of the job
 */
void jobs_add(const pid_t pid, const char *cmd) {
    if (num_jobs >= MAX_JOBS) {
        fprintf(stderr, "jobs: too many jobs\n");
        return;
    }

    if (num_jobs == 0) next_id = 1;
    else next_id++;

    jobs_table[num_jobs++] = (Job){
        .id = next_id,
        .pid = pid,
        .cmd = strdup(cmd),
        .status = JOB_RUNNING,
    };
    printf("job with id '%d' and pid: %d started.\n", jobs_table[num_jobs - 1].id, pid);
}

/**
 * prints all the jobs to the terminal
 */
void jobs_print() {
    for (int i = 0; i < num_jobs; i++) {
        if (jobs_table[i].status == JOB_RUNNING) {
            printf("%s with id %d is %s\n",
                   jobs_table[i].cmd,
                   jobs_table[i].id,
                   "running"
            );
        }
    }
}

// reap jobs and cleanup unsued jobs in the table
void jobs_reap() {
    for (int i = 0; i < num_jobs; i++) {
        if (jobs_table[i].status != JOB_RUNNING) continue;

        int wstatus;
        const pid_t result = waitpid(jobs_table[i].pid, &wstatus, WNOHANG);
        if (result == -1) {
            perror("waitpid");
            continue;
        }
        if (result == 0) continue;

        jobs_table[i].status = JOB_DONE;
        printf("job %s with id %d finished\n", jobs_table[i].cmd, jobs_table[i].id);
    }

    int alive = 0;
    for (int i = 0; i < num_jobs; i++) {
        if (jobs_table[i].status == JOB_RUNNING) {
            jobs_table[alive++] = jobs_table[i];
        } else {
            free(jobs_table[i].cmd);
        }
    }
    num_jobs = alive;
}
