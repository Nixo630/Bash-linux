#include <stdbool.h>

// Structures
struct Job {//maximum de 40 jobs simultanément
    int nJob;
    int pid;
    char* state;
    char* command_name;
    char* ground;
};
typedef struct Job Job;

// Variables globales
int nbJobs;
Job* l_jobs;
int nTimesPrintStop;

// Fonctions
void create_job(char * command_name, char *status, pid_t pid, char * ground);
void print_job(Job job);
void print_jobs(pid_t job, bool isJob, bool tHyphen);
void removeJob (int n);
bool inspectAllSons(pid_t pid3, int sig,bool print,bool hasStopped);
int killJob (char* sig, char* pid);
int bg(int job_num);
int fg(int job_num);
void check_sons_state();