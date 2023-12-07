// Structures
struct Job {
    int nJob;
    int pid;
    char* state;
    char* command_name;
};
typedef struct Job Job;

// Variables globales
static int nbJobs;
static Job* l_jobs;

// Fonctions
void print_job(Job job);
void print_jobs();
void removeJob (int n);
int killJob (char* sig, char* pid);