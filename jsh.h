typedef struct Job Job;
struct Job
{
    int nJob;
    int pid;
    char* state;
    char* command_name;
};

char* pwd();
int external_command(char** arguments,bool create_new_job, char* buffer);
int question_mark();
void callRightCommand(char**argsComm, unsigned nbArgs, char* buffer);
char* getPrompt();
void cd(char* pathname);
void exit_jsh(int val);
int main(int argc, char** argv);
void main_loop();
void reset(char** args, size_t len);
void print_job(Job job);
void print_jobs();
void copy(char* dest, char* src);

bool running;
int lastReturn;
int nbJobs;
char* current_folder;
char* previous_folder;
Job* l_jobs;