#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include "toolbox_jsh.h"
#include "jobs_jsh.h"

void print_job(Job job) {
    fprintf(stderr,"[%d] %d %s %s\n",job.nJob,job.pid,job.state,job.command_name);
}

void print_jobs() {
    for (int i = 0; i < nbJobs; i++) {
        if (l_jobs[i].pid != 0) {
            //write(STDOUT_FILENO,"bonjour",7);
            print_job(l_jobs[i]);
        }
    }
}

void removeJob (int n) {
    l_jobs[n].nJob = 0;
    l_jobs[n].pid = 0;
    free(l_jobs[n].state);
    free(l_jobs[n].command_name);
    for (int i = n; i < nbJobs; i++) {
        l_jobs[i] = l_jobs[i+1];
    }
}

int killJob (char* sig, char* pid) {
    int sig2 = convert_str_to_int(sig);
    if (sig2 == INT_MIN) {
        fprintf(stderr,"wrong command");
        return -2;
    }
    int pid2 = convert_str_to_int(pid);
    if (pid2 == INT_MIN) {
        fprintf(stderr,"wrong command");
        return -2;
    }
    int returnValue = kill(pid2,sig2);
    if (returnValue == 0 && sig2 == 9) {
        int i = 0;
        while (l_jobs[i].pid == pid2) {
            i++;
        }
        free(l_jobs[i].state);
        char* state = malloc(sizeof(char)*11);
        strcpy(state,"Terminated");
        l_jobs[i].state = state;
        print_job(l_jobs[i]);
        removeJob(i);
        nbJobs--;
    }
    return returnValue;
}