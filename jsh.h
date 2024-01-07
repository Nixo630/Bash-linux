#include <stdbool.h>

// Structures
struct Job {
    int nJob;
    int pid;
    char* state;
    char* command_name;
};
typedef struct Job Job;

// Fonctions de commande
char* pwd();
int cd(char* pathname);
int exit_jsh(int val);
void print_lastReturn();
int external_command(Command* command, int pipe_out[2]);
void print_job(Job job);
void print_jobs();
void removeJob (int n);
int killJob (char* sig, char* pid);

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void execute_command(Command* command, int pipe_out[2]);
void apply_redirections(Command* command, int pipe_in[2], int pipe_out[2]);
bool is_internal(char* command_name);
int callRightCommand(Command* command);
bool correct_nbArgs(char**, unsigned, unsigned);
char* getPrompt();

// Variables globales
bool running;
int lastReturn;
char* current_folder;
char* previous_folder;
int nbJobs;
Job* l_jobs;
