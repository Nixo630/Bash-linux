// Structures
struct Job {
    int nJob;
    pid_t pid;
    char* state;
    char* command_name;
};
typedef struct Job Job;

// Variables globales
int nbJobs;
Job* l_jobs;

// Fonctions
void print_job(Job job);
void print_jobs();
int length_nbJobs();
void removeJob (int n);
int killJob (char* sig, char* pid);