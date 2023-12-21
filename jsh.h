#include <stdbool.h>
// Fonctions de commande
char* pwd();
void cd(char* pathname);
void exit_jsh(int val);
int question_mark();
int external_command(char** arguments, bool create_new_job, char* buffer);

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void callRightCommand(char**argsComm, unsigned nbArgs, char* strComm);
bool correct_nbArgs(char**, unsigned, unsigned);
char* getPrompt();

// Variables globales
bool running;
int lastReturn;
char* current_folder;
char* previous_folder;