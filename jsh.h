// Structures
struct Job {
    int nJob;
    pid_t pid;
    char* state;
    char* command_name;
};
typedef struct Job Job;

// Fonctions de commande
char* pwd();
void cd(char* pathname);
void exit_jsh(int val);
int question_mark();
int external_command(char** arguments, bool create_new_job, char* buffer);
void print_jobs();

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void reset(char** args, size_t len);
void print_job(Job job);
void copy(char* dest, char* src);
void callRightCommand(char**argsComm, unsigned nbArgs, char* buffer);
bool correct_nbArgs(char**, unsigned, unsigned);
char* getPrompt();
void checkAlloc(void*);
void removeJob (int n);
int killJob (char* sig, char* pid);
int convert_str_to_int (char* string);

// Variables globales
bool running;
int lastReturn;
int nbJobs;
char* current_folder;
char* previous_folder;
Job* l_jobs;