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
extern int nbJobs;
extern Job* l_jobs;

// Fonctions
void create_job(char * command_name, char *status, pid_t pid, char * ground);
void print_job(Job job);
void print_jobs(pid_t job, bool isJob, bool tHyphen);
void removeJob (int n);
bool inspectAllSons(pid_t pid, int sig,bool print,bool hasStopped);
int killJob (char* sig, char* pid);
void check_sons_state();
void waitForAllSons(pid_t pid);