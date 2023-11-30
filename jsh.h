// Fonctions de commande
char* pwd();
void cd(char*);
int question_mark();
void exit_jsh(int);
int external_command(char** arguments);

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void reset(char** args, size_t);
void callRightCommand(char**, unsigned);
bool correct_nbArgs(char**, unsigned, unsigned);
char* getPrompt();

// Variables globales
bool running;
int lastReturn;
int nbJobs;
char* current_folder;
char* previous_folder;