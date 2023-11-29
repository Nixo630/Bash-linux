char* pwd();
int external_command(char** arguments);
int question_mark();
void callRightCommand(char(*)[100]);
void print_path();
void cd(char*);
void exit_jsh();
int main(int argc, char** argv);

bool running;
int lastReturn;
int nbJobs;
char* current_folder;