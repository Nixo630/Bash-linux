char* pwd();
int external_command(char** arguments);
int question_mark();
void callRightCommand(char**, unsigned);
void cd(char*);
void exit_jsh(int);
int main(int argc, char** argv);
void main_loop();
void reset(char** args, size_t);
char* getPrompt();
void testNbArguments(char**);

bool running;
int lastReturn;
int nbJobs;
char* current_folder;
char* previous_folder;