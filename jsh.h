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
void cd(char* pathname);
void exit_jsh(int val);
int question_mark();
int external_command(Command* command, char* fifo_out_name);
void print_job(Job job);
void print_jobs();
void removeJob (int n);
int killJob (char* sig, char* pid);

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void execute_command(Command* command, char* fifo_out_name);
void apply_redirections(Command* command, char* fifo_in_name, char* fifo_out_name);
bool is_internal(char* command_name);
void callRightCommand(Command* command);
bool correct_nbArgs(char**, unsigned, unsigned);
char* getPrompt();

// Variables globales
bool running;
int lastReturn;
char* current_folder;
char* previous_folder;
int nbJobs;
Job* l_jobs;
