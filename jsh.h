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
// void simple_redirection(char** argsComm, unsigned nbArgs, char* buffer,bool error, char * pathname );
// void overwritte_redirection(char** argsComm, unsigned nbArgs, char* buffer, bool error, char * pathname );
// void concat_redirection(char** argsComm, unsigned nbArgs, char* buffer, bool error, char * pathname );
// void entry_redirection(char** argsComm, unsigned nbArgs, char* buffer, char * pathname );
// void cmd_redirection (char** argsComm_1, unsigned nbArgs_1, char* buffer_1,char** argsComm_2, unsigned nbArgs_2, char* buffer_2);
void print_job(Job job);
void print_jobs();
void removeJob (int n);
int killJob (char* sig, char* pid);

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void executeCommand(Command* command, char* fifo_out_name);
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
