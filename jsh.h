#include <stdbool.h>
// Fonctions de commande
char* pwd();
void cd(char* pathname);
void exit_jsh(int val);
int question_mark();
int external_command(char** arguments, bool create_new_job, char* buffer);
void simple_redirection(char** argsComm, unsigned nbArgs, char* buffer,bool error, char * pathname );
void overwritte_redirection(char** argsComm, unsigned nbArgs, char* buffer, bool error, char * pathname );
void concat_redirection(char** argsComm, unsigned nbArgs, char* buffer, bool error, char * pathname );
void entry_redirection(char** argsComm, unsigned nbArgs, char* buffer, char * pathname );
void cmd_redirection (char** argsComm_1, unsigned nbArgs_1, char* buffer_1,char** argsComm_2, unsigned nbArgs_2, char* buffer_2);

// Fonctions auxiliaires
int main(int argc, char** argv);
void main_loop();
void callRightCommand(char**argsComm, unsigned nbArgs, char* buffer);
bool correct_nbArgs(char**, unsigned, unsigned);
char* getPrompt();

// Variables globales
bool running;
int lastReturn;
char* current_folder;
char* previous_folder;